/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 */

#ifndef __RAUC_INSTALLER_H__
#define __RAUC_INSTALLER_H__

#include <glib.h>

/**
 * @brief struct that contains authentication for bundle streaming.
 */
struct install_auth {
        gchar *header;                /**< Authentication http header */
        gchar *ssl_key;               /**< SSL client authentication key */
        gchar *ssl_cert;              /**< SSL client authentication certificate */
};

/**
 * @brief struct that contains the context of an Rauc installation.
 */
struct install_context {
        gchar *bundle;                /**< Rauc bundle file to install */
        struct install_auth *auth;    /**< Client authentication for bundle streaming */
        gboolean ssl_verify;          /**< Whether to ignore server cert verification errors */
        GSourceFunc notify_event;     /**< Callback function */
        GSourceFunc notify_complete;  /**< Callback function */
        GMutex status_mutex;          /**< Mutex used for accessing status_messages */
        GQueue status_messages;       /**< Queue of status messages from Rauc DBUS */
        gint status_result;           /**< The result of the installation */
        GMainLoop *mainloop;          /**< The installation GMainLoop  */
        GMainContext *loop_context;   /**< GMainContext for the GMainLoop */
        gboolean keep_install_context; /**< Whether the installation thread should free this struct or keep it */
};

/**
 * @brief RAUC install bundle
 *
 * @param[in] bundle RAUC bundle file (.raucb) to install.
 * @param[in] auth Authentication on HTTP streaming installation or NULL on normal installation.
 * @param[in] ssl_verify Whether to ignore server cert verification errors.
 * @param[in] on_install_notify Callback function to be called with status info during
 *                              installation.
 * @param[in] on_install_complete Callback function to be called with the result of the
 *                                installation.
 * @param[in] wait Whether to wait until install thread finished or not.
 * @return for wait=TRUE, TRUE if installation succeeded, FALSE otherwise; for
 *         wait=FALSE TRUE is always returned immediately
 */
gboolean rauc_install(const gchar *bundle, struct install_auth *auth, gboolean ssl_verify,
                GSourceFunc on_install_notify, GSourceFunc on_install_complete, gboolean wait);

#endif // __RAUC_INSTALLER_H__
