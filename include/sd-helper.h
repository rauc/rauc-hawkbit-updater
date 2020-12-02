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

#ifndef __SD_HELPER_H__
#define __SD_HELPER_H__

#include <glib.h>
#include <systemd/sd-event.h>
#include <systemd/sd-daemon.h>

/**
 * @brief Binding GSource and sd_event together.
 */
struct SDSource
{
        GSource source;
        sd_event *event;
        GPollFD pollfd;
};

/**
 * @brief Attach GSource to GMainLoop
 *
 * @param[in] source Glib GSource
 * @param[in] loop GMainLoop the GSource should be attached to.
 * @return 0 on success, value != 0 on error
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GMainLoop
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
int sd_source_attach(GSource *source, GMainLoop *loop);

/**
 * @brief Create GSource from a sd_event
 *
 * @param[in] event Systemd event that should be converted to a Glib GSource
 * @return the newly-created GSource
 * @see https://www.freedesktop.org/software/systemd/man/sd-event.html
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
GSource * sd_source_new(sd_event *event);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(sd_event, sd_event_unref)

#endif // __SD_HELPER_H__
