/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2023 Vyacheslav Yurkov <uvv.mail@gmail.com>
 */

#ifndef __RAUC_INSTALL_CONFIRMATION_H__
#define __RAUC_INSTALL_CONFIRMATION_H__

#include <glib.h>

/**
 * @brief struct that contains the context of an confirmation request.
 */
struct confirm_context {
        GSourceFunc notify_confirm;   /**< Callback function */
        GMutex status_mutex;          /**< Mutex used for accessing status_messages */
        GMainLoop *mainloop;          /**< Request's GMainLoop  */
        GMainContext *loop_context;   /**< GMainContext for the GMainLoop */
        gboolean confirmed;           /**< Confirmation status: True - confirmed, False - denied */
        gchar *action_id;             /**< Hawkbit's Action ID */
        gchar *new_version;           /**< Version string requested to be installed */
        gchar *details;               /**< Optional detailed string explaining confirmation status */
        gint error_code;              /**< Optional error code */
};

/**
 * @brief Request a confirmation about installation of a new version
 *
 * @param[in] action_id Internal action ID of the installation request. Response should use the same ID
 * @param[in] version Version string of a new bundle. The client should have own rules how to compare
 *            different version strings.
 * @param[in] on_confirm Callback function to be called when confirmation is issued
 */
void rauc_installation_confirm(const gchar *action_id, const gchar *version, GSourceFunc on_confirm);

#endif // __RAUC_INSTALL_CONFIRMATION_H__
