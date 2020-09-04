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

#ifndef __RAUC_INSTALLER_H__
#define __RAUC_INSTALLER_H__

#include <stdio.h>
#include <glib-2.0/glib.h>
#include "rauc-installer-gen.h"

/**
 * @brief struct that contains the context of an Rauc installation.
 */
struct install_context {
        gchar *bundle;                /**< Rauc bundle file to install */
        GSourceFunc notify_event;     /**< Callback function */
        GSourceFunc notify_complete;  /**< Callback function */
        GMutex status_mutex;          /**< Mutex used for accessing status_messages */
        GQueue status_messages;       /**< Queue of status messages from Rauc DBUS */
        gint status_result;           /**< The result of the installation */
        GMainLoop *mainloop;          /**< The installation GMainLoop  */
        GMainContext *loop_context;   /**< GMainContext for the GMainLoop */
};

/**
 * @brief RAUC install bundle
 *
 * @param[in] bundle RAUC bundle file (.raucb) to install.
 * @param[in] on_install_notify Callback function to be called with status info during installation.
 * @param[in] on_install_complete Callback function to be called with the result of the installation.
 */
void rauc_install(const gchar *bundle, GSourceFunc on_install_notify, GSourceFunc on_install_complete);

#endif // __RAUC_INSTALLER_H__
