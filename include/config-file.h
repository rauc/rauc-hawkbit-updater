/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 */

#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__

#include <glib.h>

/**
 * @brief struct that contains the Rauc HawkBit configuration.
 */
typedef struct Config_ {
        gchar* hawkbit_server;            /**< hawkBit host or IP and port */
        gboolean ssl;                     /**< use https or http */
        gboolean ssl_verify;              /**< verify https certificate */
        gchar* ssl_key;                   /**< SSL/TLS authentication private key */
        gchar* ssl_cert;                  /**< SSL/TLS client certificate */
        gchar* ssl_engine;                /**< SSL engine to use with ssl_key */
        gboolean post_update_reboot;      /**< reboot system after successful update */
        gboolean resume_downloads;        /**< resume downloads or not */
        gboolean stream_bundle;           /**< streaming installation or not */
        gchar* auth_token;                /**< hawkBit target security token */
        gchar* gateway_token;             /**< hawkBit gateway security token */
        gchar* tenant_id;                 /**< hawkBit tenant id */
        gchar* controller_id;             /**< hawkBit controller id*/
        gchar* bundle_download_location;  /**< file to download rauc bundle to */
        int connect_timeout;              /**< connection timeout */
        int timeout;                      /**< reply timeout */
        int retry_wait;                   /**< wait between retries */
        int low_speed_time;               /**< time to be below the speed to trigger low speed abort */
        int low_speed_rate;               /**< low speed limit to abort transfer */
        GLogLevelFlags log_level;         /**< log level */
        GHashTable* device;               /**< Additional attributes sent to hawkBit */
} Config;

/**
 * @brief Get Config for config_file.
 *
 * @param[in]  config_file String value containing path to config file
 * @param[out] error       Error
 * @return Config on success, NULL otherwise (error is set)
 */
Config* load_config_file(const gchar *config_file, GError **error);

/**
 * @brief Frees the memory allocated by a Config
 *
 * @param[in] config Config to free
 */
void config_file_free(Config *config);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Config, config_file_free)

#endif // __CONFIG_FILE_H__
