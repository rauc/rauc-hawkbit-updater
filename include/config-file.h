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
 */

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
