/**
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (C) 2018-2020 Prevas A/S (www.prevas.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * @file hawkbit-client.c
 * @author Lasse Mikkelsen <lkmi@prevas.dk>
 * @date 19 Sep 2018
 * @brief Hawkbit client
 *
 * Implementation of the hawkBit DDI API.
 *
 * @see https://github.com/rauc/rauc-hawkbit
 * @see https://www.eclipse.org/hawkbit/apis/ddi_api/
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/statvfs.h>
#include <curl/curl.h>
#include <glib-2.0/glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>
#include <libgen.h>

#include "json-helper.h"
#ifdef WITH_SYSTEMD
#include "sd-helper.h"
#endif

#include "hawkbit-client.h"

gboolean volatile force_check_run = FALSE;
gboolean run_once = FALSE;

/**
 * @brief String representation of HTTP methods.
 */
static const char *HTTPMethod_STRING[] = {
        "GET", "HEAD", "PUT", "POST", "PATCH", "DELETE"
};

static struct config *hawkbit_config = NULL;
static GSourceFunc software_ready_cb;
static gchar * volatile action_id = NULL;
static long hawkbit_interval_check_sec = DEFAULT_SLEEP_TIME_SEC;
static long sleep_time = 20;
static long last_run_sec = 0;

/**
 * @brief Get available free space
 *
 * @param[in] path Path
 * @return If error -1 else free space in bytes
 */
static gint64 get_available_space(const char* path, GError **error)
{
        struct statvfs stat;
        g_autofree gchar *npath = g_strdup(path);
        char *rpath = dirname(npath);
        if (statvfs(rpath, &stat) != 0) {
                // error happend, lets quit
                g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Failed to calculate free space: %s", g_strerror(errno));
                return -1;
        }

        // the available free space is f_bsize * f_bavail
        return (gint64) stat.f_bsize * (gint64) stat.f_bavail;
}

/**
 * @brief Curl callback used for writting software bundle file
 *        and calculate hawkbit checksum.
 *
 * @see   https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
 */
static size_t curl_write_to_file_cb(void *ptr, size_t size, size_t nmemb, struct get_binary *data)
{
        size_t written = fwrite(ptr, size, nmemb, data->fp);
        data->written += written;
        if (data->checksum) {
                g_checksum_update(data->checksum, ptr, written);
        }
        //g_debug("Bytes downloaded: %ld, (%3.f %%)", data->written, ((double) data->written / data->size * 100));
        return written;
}

/**
 * @brief download software bundle to file.
 *
 * @param[in]  download_url   URL to Software bundle
 * @param[in]  file           File the software bundle should be written to.
 * @param[in]  filesize       Expected file size
 * @param[out] checksum       Calculated checksum
 * @param[out] http_code      Return location for the http_code, can be NULL
 * @param[out] error          Error
 */
static gboolean get_binary(const gchar* download_url, const gchar* file, gint64 filesize, struct get_binary_checksum *checksum, gint *http_code, GError **error)
{
        FILE *fp = fopen(file, "wb");
        if (fp == NULL) {
                g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                            "Failed to open file for download: %s", file);
                return FALSE;
        }

        CURL *curl = curl_easy_init();
        if (!curl) {
                fclose(fp);
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            "Unable to start libcurl easy session");
                return FALSE;
        }

        struct get_binary gb = {
                .fp       = fp,
                .filesize = filesize,
                .written  = 0,
                .checksum = (checksum != NULL ? g_checksum_new(checksum->checksum_type) : NULL)
        };

        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, HAWKBIT_USERAGENT);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, hawkbit_config->connect_timeout);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, DEFAULT_CURL_DOWNLOAD_BUFFER_SIZE);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_file_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &gb);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, hawkbit_config->ssl_verify ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, hawkbit_config->ssl_verify ? 1L : 0L);
        /* abort if slower than 100 bytes/sec during 60 seconds */
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 100L);
        // Setup request headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/octet-stream");
        if (hawkbit_config->auth_token) {
                g_autofree gchar* auth_token = g_strdup_printf("Authorization: TargetToken %s", hawkbit_config->auth_token);
                headers = curl_slist_append(headers, auth_token);
        } else if (hawkbit_config->gateway_token) {
                g_autofree gchar* gateway_token = g_strdup_printf("Authorization: GatewayToken %s", hawkbit_config->gateway_token);
                headers = curl_slist_append(headers, gateway_token);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if (http_code)
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
        if (res == CURLE_OK) {
                if (gb.checksum) { // if checksum enabled then return the value
                        checksum->checksum_result = g_strdup(g_checksum_get_string(gb.checksum));
                        g_checksum_free(gb.checksum);
                }
        } else {
                g_set_error(error,
                            G_IO_ERROR,                    // error domain
                            G_IO_ERROR_FAILED,             // error code
                            "HTTP request failed: %s",     // error message format string
                            curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        fclose(fp);
        return (res == CURLE_OK);
}

/**
 * @brief Curl callback used for writting rest response to buffer.
 */
static size_t curl_write_cb(void *content, size_t size, size_t nmemb, void *data)
{
        struct rest_payload *p = (struct rest_payload *) data;
        size_t real_size = size * nmemb;

        p->payload = (gchar *) g_realloc(p->payload, p->size + real_size + 1);
        if (p->payload == NULL) {
                g_debug("Failed to expand buffer");
                return -1;
        }

        // copy content to buffer
        memcpy(&(p->payload[p->size]), content, real_size);
        p->size += real_size;
        p->payload[p->size] = '\0';

        return real_size;
}

/**
 * @brief Make REST request.
 *
 * @param[in]  method             HTTP Method ex. GET
 * @param[in]  url                URL used in HTTP REST request
 * @param[in]  jsonRequestBody    REST request body. If NULL, no body is sent.
 * @param[out] jsonResponseParser REST response
 * @param[out] error              Error
 * @return HTTP Status code (Standard codes: 200 = OK, 524 = Operation timed out, 401 = Authorization needed, 403 = Authentication failed )
 */
static gint rest_request(enum HTTPMethod method, const gchar* url, JsonBuilder* jsonRequestBody, JsonParser** jsonResponseParser, GError** error)
{
        gchar *postdata = NULL;
        struct rest_payload fetch_buffer;

        CURL *curl = curl_easy_init();
        if (!curl) return -1;

        // init response buffer
        fetch_buffer.payload = g_malloc0(DEFAULT_CURL_REQUEST_BUFFER_SIZE);
        if (fetch_buffer.payload == NULL) {
                g_debug("Failed to expand buffer");
                curl_easy_cleanup(curl);
                return -1;
        }
        fetch_buffer.size = 0;

        // setup CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, HAWKBIT_USERAGENT);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, HTTPMethod_STRING[method]);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, hawkbit_config->connect_timeout);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, hawkbit_config->timeout);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &fetch_buffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, hawkbit_config->ssl_verify ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, hawkbit_config->ssl_verify ? 1L : 0L);

        if (jsonRequestBody) {
                // Convert request into a string
                JsonGenerator *generator = json_generator_new();
                json_generator_set_root(generator, json_builder_get_root(jsonRequestBody));
                gsize length;
                postdata = json_generator_to_data(generator, &length);
                g_object_unref(generator);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
                g_debug("Request body: %s\n", postdata);
        }

        // Setup request headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json;charset=UTF-8");
        if (hawkbit_config->auth_token) {
                g_autofree gchar* auth_token = g_strdup_printf("Authorization: TargetToken %s", hawkbit_config->auth_token);
                headers = curl_slist_append(headers, auth_token);
        } else if (hawkbit_config->gateway_token) {
                g_autofree gchar* gateway_token = g_strdup_printf("Authorization: GatewayToken %s", hawkbit_config->gateway_token);
                headers = curl_slist_append(headers, gateway_token);
        }
        if (jsonRequestBody) {
                headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // perform request
        CURLcode res = curl_easy_perform(curl);
        int http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (res == CURLE_OK && http_code == 200) {
                if (jsonResponseParser && fetch_buffer.size > 0) {
                        JsonParser *parser = json_parser_new_immutable();
                        if (json_parser_load_from_data(parser, fetch_buffer.payload, fetch_buffer.size, error)) {
                                *jsonResponseParser = parser;
                        } else {
                                g_object_unref(parser);
                                g_debug("Failed to parse JSON response body. status: %d\n", http_code);
                        }
                }
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
                // libcurl was able to complete a TCP connection to the origin server, but did not receive a timely HTTP response.
                http_code = 524;
                g_set_error(error,
                            1,                    // error domain
                            http_code,
                            "HTTP request timed out: %s",
                            curl_easy_strerror(res));
        } else {
                g_set_error(error,
                            1,                    // error domain
                            http_code,
                            "HTTP request failed: %s",
                            curl_easy_strerror(res));
        }

        //g_debug("Response body: %s\n", fetch_buffer.payload);

        g_free(fetch_buffer.payload);
        g_free(postdata);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return http_code;
}

/**
 * @brief Build JSON status request.
 * @see https://www.eclipse.org/hawkbit/rest-api/rootcontroller-api-guide/#_post_tenant_controller_v1_controllerid_deploymentbase_actionid_feedback
 */
static void json_build_status(JsonBuilder *builder, const gchar *id, const gchar *detail, const gchar *result, const gchar *execution, GHashTable *data, gint progress)
{
        GHashTableIter iter;
        gpointer key, value;

        // Get current time in UTC
        time_t current_time;
        struct tm time_info;
        char timeString[16];
        time(&current_time);
        gmtime_r(&current_time, &time_info);
        strftime(timeString, sizeof(timeString), "%Y%m%dT%H%M%S", &time_info);

        // build json status
        json_builder_begin_object(builder);

        if (id) {
                json_builder_set_member_name(builder, "id");
                json_builder_add_string_value(builder, id);
        }

        json_builder_set_member_name(builder, "time");
        json_builder_add_string_value(builder, timeString);

        json_builder_set_member_name(builder, "status");
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "result");
        json_builder_begin_object(builder);

        if (g_strcmp0(execution, "proceeding") == 0) {
                json_builder_set_member_name(builder, "progress");
                json_builder_begin_object(builder);

                json_builder_set_member_name(builder, "of");
                json_builder_add_int_value(builder, 1);

                json_builder_set_member_name(builder, "cnt");
                json_builder_add_int_value(builder, 3);

                json_builder_end_object(builder);
        }

        json_builder_set_member_name(builder, "finished");
        json_builder_add_string_value(builder, result);
        json_builder_end_object(builder);

        json_builder_set_member_name(builder, "execution");
        json_builder_add_string_value(builder, execution);

        if (detail) {
                json_builder_set_member_name(builder, "details");
                json_builder_begin_array(builder);
                json_builder_add_string_value(builder, detail);
                json_builder_end_array(builder);
        }
        json_builder_end_object(builder);

        if (data) {
                json_builder_set_member_name(builder, "data");
                json_builder_begin_object(builder);
                g_hash_table_iter_init(&iter, data);
                while (g_hash_table_iter_next(&iter, &key, &value)) {
                        json_builder_set_member_name(builder, key);
                        json_builder_add_string_value(builder, value);
                }
                json_builder_end_object(builder);
        }
        json_builder_end_object(builder);
}

/**
 * @brief Send feedback to hawkBit.
 */
static gboolean feedback(gchar *url, gchar *id, gchar *detail, gchar *finished, gchar *execution, GError **error)
{
        JsonBuilder *builder = json_builder_new();
        json_build_status(builder, id, detail, finished, execution, NULL, 0);

        int status = rest_request(POST, url, builder, NULL, error);
        g_debug("Feedback status: %d, URL: %s", status, url);
        g_object_unref(builder);
        return (status == 200);
}

/**
 * @brief Send progress feedback to hawkBit.
 */
static gboolean feedback_progress(const gchar *url, const gchar *id, gint progress, const gchar *detail, GError **error)
{
        JsonBuilder *builder = json_builder_new();
        json_build_status(builder, id, detail, "none", "proceeding", NULL, progress);

        int status = rest_request(POST, url, builder, NULL, error);
        g_debug("Feedback progress status: %d, URL: %s", status, url);
        g_object_unref(builder);
        return (status == 200);
}

/**
 * @brief Get polling sleep time from hawkBit JSON response.
 */
static long json_get_sleeptime(JsonNode *root)
{
        gchar *sleeptime_str = json_get_string(root, "$.config.polling.sleep");
        if (sleeptime_str) {
                struct tm time;
                strptime(sleeptime_str, "%T", &time);
                long poll_sleep_time = (time.tm_sec + (time.tm_min * 60) + (time.tm_hour * 60 * 60));
                //g_debug("sleep time: %s %ld\n", sleeptime_str, poll_sleep_time);
                g_free(sleeptime_str);
                return poll_sleep_time;
        }
        return DEFAULT_SLEEP_TIME_SEC;
}

/**
 * @brief
 */
static gchar** regex_groups(const gchar* pattern, const gchar *str, GError **error)
{
        gchar **result = NULL;
        GMatchInfo *match_info;
        GRegex *regex = g_regex_new(pattern, 0, 0, error);
        g_regex_match(regex, str, 0, &match_info);
        if (g_match_info_matches(match_info))
        {
                result = g_match_info_fetch_all(match_info);
        }
        g_match_info_free(match_info);
        g_regex_unref(regex);
        return result;
}

/**
 * @brief Build API URL
 */
static gchar* build_api_url(gchar *path)
{
        return g_strdup_printf("%s://%s%s", hawkbit_config->ssl ? "https" : "http", hawkbit_config->hawkbit_server, path);
}

gboolean hawkbit_progress(const gchar *msg)
{
        g_autofree gchar *feedback_url = NULL;
        g_autofree gchar *path = NULL;
        if (action_id) {
                path = g_strdup_printf("/%s/controller/v1/%s/deploymentBase/%s/feedback", hawkbit_config->tenant_id,
                                       hawkbit_config->controller_id, action_id);
                feedback_url = build_api_url(path);
                feedback_progress(feedback_url, action_id, 3, msg, NULL);
        }
        return G_SOURCE_REMOVE;
}

static gboolean identify(GError **error)
{
        g_debug("Identifying ourself to hawkbit server");
        g_autofree gchar *put_config_data_url = build_api_url(
                g_strdup_printf("/%s/controller/v1/%s/configData", hawkbit_config->tenant_id,
                                hawkbit_config->controller_id));

        JsonBuilder *builder = json_builder_new();
        json_build_status(builder, NULL, NULL, "success", "closed", hawkbit_config->device, FALSE);

        gint status = rest_request(PUT, put_config_data_url, builder, NULL, error);
        g_object_unref(builder);
        return (status == 200);
}

static void process_artifact_cleanup(struct artifact *artifact)
{
        if (artifact == NULL)
                return;
        g_free(artifact->name);
        g_free(artifact->version);
        g_free(artifact->download_url);
        g_free(artifact->feedback_url);
        g_free(artifact->sha1);
        g_free(artifact->md5);
        g_free(artifact);
}

static void process_deployment_cleanup()
{
        //g_clear_pointer(action_id, g_free);
        gpointer ptr = action_id;
        action_id = NULL;
        g_free(ptr);

        if (g_file_test(hawkbit_config->bundle_download_location, G_FILE_TEST_EXISTS)) {
                if (g_remove(hawkbit_config->bundle_download_location) != 0) {
                        g_debug("Failed to delete file: %s", hawkbit_config->bundle_download_location);
                }
        }
}

gboolean install_complete_cb(gpointer ptr)
{
        struct on_install_complete_userdata *result = ptr;
        g_autofree gchar *feedback_url = NULL;
        g_autofree gchar *path = NULL;
        if (action_id) {
                path = g_strdup_printf("/%s/controller/v1/%s/deploymentBase/%s/feedback", hawkbit_config->tenant_id,
                                       hawkbit_config->controller_id, action_id);
                feedback_url = build_api_url(path);
                if (result->install_success) {
                        g_message("Software bundle installed successful.");
                        feedback(feedback_url, action_id, "Software bundle installed successful.", "success", "closed", NULL);
                } else {
                        g_critical("Failed to install software bundle.");
                        feedback(feedback_url, action_id, "Failed to install software bundle.", "failure", "closed", NULL);
                }

                process_deployment_cleanup();
        }
        return G_SOURCE_REMOVE;
}

static gpointer download_thread(gpointer data)
{
        struct on_new_software_userdata userdata = {
                .install_progress_callback = (GSourceFunc) hawkbit_progress,
                .install_complete_callback = install_complete_cb,
                .file = hawkbit_config->bundle_download_location,
        };

        GError *error = NULL;
        g_autofree gchar *msg = NULL;
        struct artifact *artifact = data;
        g_message("Start downloading: %s", artifact->download_url);

        // setup checksum
        struct get_binary_checksum checksum = { .checksum_result = NULL, .checksum_type = G_CHECKSUM_SHA1 };

        // Download software bundle (artifact)
        gint64 start_time = g_get_monotonic_time();
        gint status = 0;
        gboolean res = get_binary(artifact->download_url, hawkbit_config->bundle_download_location,
                                  artifact->size, &checksum, &status, &error);
        gint64 end_time = g_get_monotonic_time();
        if (!res) {
                g_autofree gchar *msg = g_strdup_printf("Download failed: %s Status: %d", error->message, status);
                g_clear_error(&error);
                g_critical("%s", msg);
                feedback(artifact->feedback_url, action_id, msg, "failure", "closed", NULL);
                goto down_error;
        }

        // notify hawkbit that download is complete
        msg = g_strdup_printf("Download complete. %.2f MB/s",
                              (artifact->size / ((double)(end_time - start_time) / 1000000)) / (1024 * 1024));
        feedback_progress(artifact->feedback_url, action_id, 1, msg, NULL);
        g_message("%s", msg);

        // validate checksum
        if (g_strcmp0(artifact->sha1, checksum.checksum_result)) {
                g_autofree gchar *msg = g_strdup_printf(
                        "Software: %s V%s. Invalid checksum: %s expected %s",
                        artifact->name, artifact->version,
                        checksum.checksum_result,
                        artifact->sha1);
                feedback(artifact->feedback_url, action_id, msg, "failure", "closed", NULL);
                g_critical("%s", msg);
                status = -3;
                goto down_error;
        }
        g_message("File checksum OK.");
        feedback_progress(artifact->feedback_url, action_id, 2, "File checksum OK.", NULL);
        g_free(checksum.checksum_result);
        process_artifact_cleanup(artifact);

        software_ready_cb(&userdata);
        return NULL;
down_error:
        g_free(checksum.checksum_result);
        process_artifact_cleanup(artifact);
        process_deployment_cleanup();
        return NULL;
}

static gboolean process_deployment(JsonNode *req_root, GError **error)
{
        GError *ierror = NULL;
        struct artifact *artifact = NULL;
        g_autofree gchar *str = NULL;

        if (action_id) {
                g_warning("Deployment is already in progress...");
                return FALSE;
        }

        // get deployment url
        gchar *deployment = json_get_string(req_root, "$._links.deploymentBase.href");
        if (deployment == NULL) {
                g_set_error(error,1,1,"Failed to parse deployment base response.");
                return FALSE;
        }

        // get resource id and action id from url
        gchar** groups = regex_groups("/deploymentBase/(.+)[?]c=(.+)$", deployment, NULL);
        if (groups == NULL) {
                g_set_error(error,1,2,"Failed to parse deployment base response.");
                return FALSE;
        }
        action_id = g_strdup(groups[1]);
        g_autofree gchar *resource_id = g_strdup(groups[2]);
        g_strfreev(groups);
        g_free(deployment);

        // build urls for deployment resource info
        g_autofree gchar *get_resource_url = build_api_url(
                g_strdup_printf("/%s/controller/v1/%s/deploymentBase/%s?c=%s", hawkbit_config->tenant_id,
                                hawkbit_config->controller_id, action_id, resource_id));
        gchar *feedback_url = build_api_url(
                g_strdup_printf("/%s/controller/v1/%s/deploymentBase/%s/feedback", hawkbit_config->tenant_id,
                                hawkbit_config->controller_id, action_id));

        // get deployment resource
        JsonParser *json_response_parser = NULL;
        int status = rest_request(GET, get_resource_url, NULL, &json_response_parser, error);
        if (status != 200 || json_response_parser == NULL) {
                g_debug("Failed to get resource from hawkbit server. Status: %d", status);
                goto proc_error;
        }
        JsonNode *resp_root = json_parser_get_root(json_response_parser);

        str = json_to_string(resp_root, TRUE);
        g_debug("Deployment response: %s\n", str);

        JsonArray *json_chunks = json_get_array(resp_root, "$.deployment.chunks");
        if (json_chunks == NULL || json_array_get_length(json_chunks) == 0) {
                feedback(feedback_url, action_id, "Failed to parse deployment resource.", "failure", "closed", NULL);
                g_set_error(error,1,20,"Failed to parse deployment resource.");
                goto proc_error;
        }

        // Downloading multiple chunks not supported. Only first chunk is downloaded (RAUC bundle)
        JsonNode *json_chunk = json_array_get_element(json_chunks, 0);
        JsonArray *json_artifacts = json_get_array(json_chunk, "$.artifacts");
        if (json_artifacts == NULL || json_array_get_length(json_artifacts) == 0) {
                feedback(feedback_url, action_id, "Failed to parse deployment resource.", "failure", "closed", NULL);
                g_set_error(error,1,21,"Failed to parse deployment resource.");
                goto proc_error;
        }
        JsonNode *json_artifact = json_array_get_element(json_artifacts, 0);

        // get artifact information
        artifact = g_new0(struct artifact, 1);
        artifact->version = json_get_string(json_chunk, "$.version");
        artifact->name = json_get_string(json_chunk, "$.name");
        artifact->size = json_get_int(json_artifact, "$.size");
        artifact->sha1 = json_get_string(json_artifact, "$.hashes.sha1");
        artifact->md5 = json_get_string(json_artifact, "$.hashes.md5");
        artifact->feedback_url = feedback_url;
        // favour https download
        artifact->download_url = json_get_string(json_artifact, "$._links.download.href");
        if (artifact->download_url == NULL)
                artifact->download_url = json_get_string(json_artifact, "$._links.download-http.href");

        if (artifact->download_url == NULL) {
                feedback(feedback_url, action_id, "Failed to parse deployment resource.", "failure", "closed", NULL);
                g_set_error(error,1,22,"Failed to parse deployment resource.");
                goto proc_error;
        }

        g_message("New software ready for download. (Name: %s, Version: %s, Size: %" G_GINT64_FORMAT ", URL: %s)",
                  artifact->name, artifact->version, artifact->size, artifact->download_url);

        // Check if there is enough free diskspace
        gint64 freespace = get_available_space(hawkbit_config->bundle_download_location, &ierror);
        if (freespace == -1) {
                feedback(feedback_url, action_id, ierror->message, "failure", "closed", NULL);
                g_propagate_error(error, ierror);
                status = -4;
                goto proc_error;
        } else if (freespace < artifact->size) {
                g_autofree gchar *msg = g_strdup_printf("Not enough free space. File size: %" G_GINT64_FORMAT  ". Free space: %" G_GINT64_FORMAT,
                                                        artifact->size, freespace);
                g_debug("%s", msg);
                // Notify hawkbit that there is not enough free space.
                feedback(feedback_url, action_id, msg, "failure", "closed", NULL);
                g_set_error(error, 1, 23, "%s", msg);
                status = -4;
                goto proc_error;
        }

        // start download thread
        g_thread_new("downloader", download_thread, (gpointer) artifact);

        g_object_unref(json_response_parser);
        return TRUE;

proc_error:
        g_object_unref(json_response_parser);
        // Lets cleanup processing deployment failed
        process_artifact_cleanup(artifact);
        process_deployment_cleanup();
        return FALSE;
}


void hawkbit_init(struct config *config, GSourceFunc on_install_ready)
{
        hawkbit_config = config;
        software_ready_cb = on_install_ready;
        curl_global_init(CURL_GLOBAL_ALL);
}

typedef struct ClientData_ {
        GMainLoop *loop;
        gboolean res;
} ClientData;

static gboolean hawkbit_pull_cb(gpointer user_data)
{
        ClientData *data = user_data;

        if (!force_check_run && ++last_run_sec < sleep_time)
                return G_SOURCE_CONTINUE;

        force_check_run = FALSE;
        last_run_sec = 0;

        // build hawkBit get tasks URL
        g_autofree gchar *get_tasks_url = build_api_url(
                g_strdup_printf("/%s/controller/v1/%s", hawkbit_config->tenant_id,
                                hawkbit_config->controller_id));
        GError *error = NULL;
        JsonParser *json_response_parser = NULL;

        g_message("Checking for new software...");
        int status = rest_request(GET, get_tasks_url, NULL, &json_response_parser, &error);
        if (status == 200) {
                if (json_response_parser) {
                        // json_root is owned by the JsonParser and should never be modified or freed.
                        JsonNode *json_root = json_parser_get_root(json_response_parser);
                        g_autofree gchar *str = json_to_string(json_root, TRUE);
                        g_debug("Deployment response: %s\n", str);

                        // get hawkbit sleep time (how often should we check for new software)
                        hawkbit_interval_check_sec = json_get_sleeptime(json_root);

                        if (json_contains(json_root, "$._links.configData")) {
                                // hawkBit has asked us to identify ourself
                                identify(&error);
                        }
                        if (json_contains(json_root, "$._links.deploymentBase")) {
                                // hawkBit has a new deployment for us
                                process_deployment(json_root, &error);
                        } else {
                                g_message("No new software.");
                        }
                        if (json_contains(json_root, "$._links.cancelAction")) {
                                //TODO: implement me
                                g_warning("cancel action not supported");
                        }

                        g_object_unref(json_response_parser);
                }

                if (error)
                        g_critical("Error: %s", error->message);

                // sleep as long as specified by hawkbit
                sleep_time = hawkbit_interval_check_sec;
        } else if (status == 401) {
                if (hawkbit_config->auth_token) {
                        g_critical("Failed to authenticate. Check if auth_token is correct?");
                } else if (hawkbit_config->gateway_token) {
                        g_critical("Failed to authenticate. Check if gateway_token is correct?");
                }

                sleep_time = hawkbit_config->retry_wait;
        } else {
                g_debug("Scheduled check for new software failed status code: %d", status);
                if (error) {
                        g_critical("HTTP Error: %s", error->message);
                }
                sleep_time = hawkbit_config->retry_wait;
        }
        g_clear_error(&error);

        if (run_once) {
                data->res = status == 200 ? 0 : 1;
                g_main_loop_quit(data->loop);
                return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
}

int hawkbit_start_service_sync()
{
        GMainContext *ctx;
        ClientData cdata;
        GSource *timeout_source = NULL;
        int res = 0;

        ctx = g_main_context_new();
        cdata.loop = g_main_loop_new(ctx, FALSE);

        timeout_source = g_timeout_source_new(1000);   // pull every second
        g_source_set_name(timeout_source, "Add timeout");
        g_source_set_callback(timeout_source, (GSourceFunc) hawkbit_pull_cb, &cdata, NULL);
        g_source_attach(timeout_source, ctx);
        g_source_unref(timeout_source);

#ifdef WITH_SYSTEMD
        GSource *event_source = NULL;
        sd_event *event = NULL;
        res = sd_event_default(&event);
        if (res < 0)
                goto finish;
        // Enable automatic service watchdog support
        res = sd_event_set_watchdog(event, TRUE);
        if (res < 0)
                goto finish;

        event_source = sd_source_new(event);
        if (!event_source) {
                res = -ENOMEM;
                goto finish;
        }

        // attach systemd source to glib mainloop
        res = sd_source_attach(event_source, cdata.loop);
        if (res < 0)
                goto finish;

        sd_notify(0, "READY=1\nSTATUS=Init completed, start polling HawkBit for new software.");
#endif

        g_main_loop_run(cdata.loop);

        res = cdata.res;

#ifdef WITH_SYSTEMD
        sd_notify(0, "STOPPING=1\nSTATUS=Stopped polling HawkBit for new software.");
#endif

#ifdef WITH_SYSTEMD
finish:
        g_source_unref(event_source);
        g_source_destroy(event_source);
        sd_event_set_watchdog(event, FALSE);
        event = sd_event_unref(event);
#endif
        g_main_loop_unref(cdata.loop);
        g_main_context_unref(ctx);
        if (res < 0)
                g_warning("Failure: %s\n", strerror(-res));

        return res;
}

