#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__

#include <stdlib.h>
#include <glib-2.0/glib.h>
#include <glib/gprintf.h>

/**
 * @brief struct that contains the Rauc HawkBit configuration.
 */
struct config {
        gchar* hawkbit_server;            /**< hawkBit host or IP and port */
        gboolean ssl;                     /**< use https or http */
        gboolean ssl_verify;              /**< verify https certificate */
        gchar* auth_token;                /**< hawkBit security token */
        gchar* tenant_id;                 /**< hawkBit tenant id */
        gchar* controller_id;             /**< hawkBit controller id*/
        gchar* bundle_download_location;  /**< file to download rauc bundle to */
        long connect_timeout;             /**< connection timeout */
        long timeout;                     /**< reply timeout */
        int retry_wait;                   /**< wait between retries */
        GLogLevelFlags log_level;         /**< log level */
        GHashTable* device;               /**< Additional attributes sent to hawkBit */
};

struct config* load_config_file(const gchar* config_file, GError** error);
void config_file_free(struct config *config);

#endif // __CONFIG_FILE_H__
