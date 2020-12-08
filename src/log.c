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
 * @file   log.c
 * @author Lasse Mikkelsen <lkmi@prevas.dk>
 * @date   19 Sep 2018
 * @brief  Log handling
 */

#include "log.h"
#include <stddef.h>

static gboolean output_to_systemd = FALSE;

/**
 * @brief convert GLogLevelFlags to string
 *
 * @param[in] level Log level that should be returned as string.
 * @return log level string
 */
static const gchar *log_level_to_string(GLogLevelFlags level)
{
        switch (level) {
        case G_LOG_LEVEL_ERROR:
                return "ERROR";
        case G_LOG_LEVEL_CRITICAL:
                return "CRITICAL";
        case G_LOG_LEVEL_WARNING:
                return "WARNING";
        case G_LOG_LEVEL_MESSAGE:
                return "MESSAGE";
        case G_LOG_LEVEL_INFO:
                return "INFO";
        case G_LOG_LEVEL_DEBUG:
                return "DEBUG";
        default:
                return "UNKNOWN";
        }
}

/**
 * @brief     map glib log level to syslog
 *
 * @param[in] level Log level that should be returned as string.
 * @return    syslog level
 */
#ifdef WITH_SYSTEMD
static int log_level_to_int(GLogLevelFlags level)
{
        switch (level) {
        case G_LOG_LEVEL_ERROR:
                return LOG_ERR;
        case G_LOG_LEVEL_CRITICAL:
                return LOG_CRIT;
        case G_LOG_LEVEL_WARNING:
                return LOG_WARNING;
        case G_LOG_LEVEL_MESSAGE:
                return LOG_NOTICE;
        case G_LOG_LEVEL_INFO:
                return LOG_INFO;
        case G_LOG_LEVEL_DEBUG:
                return LOG_DEBUG;
        default:
                return LOG_INFO;
        }
}
#endif

/**
 * @brief     Glib log handler callback
 *
 * @param[in] log_domain Log domain
 * @param[in] log_level  Log level
 * @param[in] message    Log message
 * @param[in] user_data  Not used
 */
static void log_handler_cb(const gchar    *log_domain,
                           GLogLevelFlags log_level,
                           const gchar    *message,
                           gpointer user_data)
{
        const gchar *log_level_str;
#ifdef WITH_SYSTEMD
        if (output_to_systemd) {
                int log_level_int = log_level_to_int(log_level & G_LOG_LEVEL_MASK);
                sd_journal_print(log_level_int, "%s", message);
        } else {
#endif
        log_level_str = log_level_to_string(log_level & G_LOG_LEVEL_MASK);
        if (log_level <= G_LOG_LEVEL_WARNING) {
                g_printerr("%s: %s\n", log_level_str, message);
        } else {
                g_print("%s: %s\n", log_level_str, message);
        }
#ifdef WITH_SYSTEMD
}
#endif
}

void setup_logging(const gchar *domain, GLogLevelFlags level, gboolean p_output_to_systemd)
{
        output_to_systemd = p_output_to_systemd;
        g_log_set_handler(NULL,
                          level | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                          log_handler_cb, NULL);
}
