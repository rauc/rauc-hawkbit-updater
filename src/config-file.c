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
 * @file config-file.c
 * @author Lasse Mikkelsen <lkmi@prevas.dk>
 * @date 19 Sep 2018
 * @brief Configuration file parser
 *
 */

#include "config-file.h"

static const gint DEFAULT_CONNECTTIMEOUT  = 20;     // 20 sec.
static const gint DEFAULT_TIMEOUT         = 60;     // 1 min.
static const gint DEFAULT_RETRY_WAIT      = 5 * 60; // 5 min.
static const gboolean DEFAULT_SSL         = TRUE;
static const gboolean DEFAULT_SSL_VERIFY  = TRUE;
static const gchar * DEFAULT_LOG_LEVEL    = "message";

void config_file_free(struct config *config)
{
        g_free(config->hawkbit_server);
        g_free(config->controller_id);
        g_free(config->tenant_id);
        g_free(config->auth_token);
        g_free(config->gateway_token);
        g_free(config->bundle_download_location);
        g_hash_table_destroy(config->device);
}

static gboolean get_key_string(GKeyFile *key_file, const gchar* group, const gchar* key, gchar** value, const gchar* default_value, GError **error)
{
        gchar *val = NULL;
        val = g_key_file_get_string(key_file, group, key, NULL);
        if (val == NULL) {
                if (default_value != NULL) {
                        *value = g_strdup(default_value);
                        return TRUE;
                }

                g_set_error(error, G_KEY_FILE_ERROR,
                            G_KEY_FILE_ERROR_NOT_FOUND,
                            "Key '%s' not found in group '%s' and no default given", key, group);
                return FALSE;
        }
        *value = val;
        return TRUE;
}

static gboolean get_key_bool(GKeyFile *key_file, const gchar* group, const gchar* key, gboolean* value, const gboolean default_value, GError **error)
{
        g_autofree gchar *val = NULL;
        val = g_key_file_get_string(key_file, group, key, NULL);
        if (val == NULL) {
                *value = default_value;
                return TRUE;
        }
        gboolean val_false = (g_strcmp0(val, "0") == 0 || g_ascii_strcasecmp(val, "no") == 0 || g_ascii_strcasecmp(val, "false") == 0);
        if (val_false) {
                *value = FALSE;
                return TRUE;
        }
        gboolean val_true = (g_strcmp0(val, "1") == 0 || g_ascii_strcasecmp(val, "yes") == 0 || g_ascii_strcasecmp(val, "true") == 0);
        if (val_true) {
                *value = TRUE;
                return TRUE;
        }

        g_set_error(error, G_KEY_FILE_ERROR,
                    G_KEY_FILE_ERROR_INVALID_VALUE,
                    "Value '%s' cannot be interpreted as a boolean.", val);

        return FALSE;
}

static gboolean get_key_int(GKeyFile *key_file, const gchar* group, const gchar* key, gint* value, const gint default_value, GError **error)
{
        GError *ierror = NULL;
        gint val = g_key_file_get_integer(key_file, group, key, &ierror);

        if (val == 0 && g_error_matches(ierror, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_clear_error(&ierror);
                *value = default_value;
                return TRUE;
        }
        else if (val == 0 && ierror) {
                g_propagate_error(error, ierror);
                return FALSE;
        }
        *value = val;
        return TRUE;
}

static gboolean get_group(GKeyFile *key_file, const gchar *group, GHashTable **hash, GError **error)
{
        guint key;
        gsize num_keys;
        gchar **keys;

        *hash = g_hash_table_new(g_str_hash, g_str_equal);
        keys = g_key_file_get_keys(key_file, group, &num_keys, error);
        if (keys == NULL)
                return FALSE;

        if (num_keys == 0) {
                g_set_error(error, G_KEY_FILE_ERROR,
                            G_KEY_FILE_ERROR_PARSE,
                            "Group '%s' has no keys set", group);
                return FALSE;
        }

        for (key = 0; key < num_keys; key++)
        {
                gchar *value = g_key_file_get_value(key_file,
                                                    group,
                                                    keys[key],
                                                    error);
                if (value == NULL)
                        return FALSE;

                g_hash_table_insert(*hash, keys[key], value);
                //g_debug("\t\tkey %u/%lu: \t%s => %s\n", key, num_keys - 1, keys[key], value);
        }

        return TRUE;
}

static GLogLevelFlags log_level_from_string(const gchar *log_level)
{
        if (g_strcmp0(log_level, "error") == 0) {
                return G_LOG_LEVEL_ERROR;
        } else if (g_strcmp0(log_level, "critical") == 0) {
                return G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL;
        } else if (g_strcmp0(log_level, "warning") == 0) {
                return G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
                       G_LOG_LEVEL_WARNING;
        } else if (g_strcmp0(log_level, "message") == 0) {
                return G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
                       G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE;
        } else if (g_strcmp0(log_level, "info") == 0) {
                return G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
                       G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE |
                       G_LOG_LEVEL_INFO;
        } else if (g_strcmp0(log_level, "debug") == 0) {
                return G_LOG_LEVEL_MASK;
        } else {
                return G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
                       G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE;
        }
}

struct config* load_config_file(const gchar* config_file, GError** error)
{
        struct config *config = g_new0(struct config, 1);

        gint val_int;
        g_autofree gchar *val = NULL;
        g_autoptr(GKeyFile) ini_file = g_key_file_new();
        gboolean key_auth_token_exists = FALSE;
        gboolean key_gateway_token_exists = FALSE;

        if (!g_key_file_load_from_file(ini_file, config_file, G_KEY_FILE_NONE, error)) {
                return NULL;
        }

        if (!get_key_string(ini_file, "client", "hawkbit_server", &config->hawkbit_server, NULL, error))
                return NULL;

        key_auth_token_exists = get_key_string(ini_file, "client", "auth_token", &config->auth_token, NULL, NULL);
        key_gateway_token_exists = get_key_string(ini_file, "client", "gateway_token", &config->gateway_token, NULL, NULL);
        if (!key_auth_token_exists && !key_gateway_token_exists) {
                g_set_error(error, 1, 4, "Neither auth_token nor gateway_token is set in the config.");
                return NULL;
        } else if (key_auth_token_exists && key_gateway_token_exists) {
                g_warning("Both auth_token and gateway_token are set in the config.");
        }

        if (!get_key_string(ini_file, "client", "target_name", &config->controller_id, NULL, error))
                return NULL;
        if (!get_key_string(ini_file, "client", "tenant_id", &config->tenant_id, "DEFAULT", error))
                return NULL;
        if (!get_key_string(ini_file, "client", "bundle_download_location", &config->bundle_download_location, NULL, error))
                return NULL;
        if (!get_key_bool(ini_file, "client", "ssl", &config->ssl, DEFAULT_SSL, error))
                return NULL;
        if (!get_key_bool(ini_file, "client", "ssl_verify", &config->ssl_verify, DEFAULT_SSL_VERIFY, error))
                return NULL;
        if (!get_group(ini_file, "device", &config->device, error))
                return NULL;

        if (!get_key_int(ini_file, "client", "connect_timeout", &val_int, DEFAULT_CONNECTTIMEOUT, error))
                return NULL;
        config->connect_timeout = val_int;

        if (!get_key_int(ini_file, "client", "timeout", &val_int, DEFAULT_TIMEOUT, error))
                return NULL;
        config->timeout = val_int;

        if (!get_key_int(ini_file, "client", "retry_wait", &val_int, DEFAULT_RETRY_WAIT, error))
                return NULL;
        config->retry_wait = val_int;

        if (!get_key_string(ini_file, "client", "log_level", &val, DEFAULT_LOG_LEVEL, error))
                return NULL;
        config->log_level = log_level_from_string(val);

        if (config->timeout > 0 && config->connect_timeout > 0 && config->timeout < config->connect_timeout) {
                g_set_error(error,
                            G_KEY_FILE_ERROR,                   // error domain
                            G_KEY_FILE_ERROR_INVALID_VALUE,     // error code
                            "timeout should be greater than connect_timeout. Timeout: %ld, Connect timeout: %ld",
                            config->timeout,
                            config->connect_timeout
                            );
                return NULL;
        }

        return config;
}
