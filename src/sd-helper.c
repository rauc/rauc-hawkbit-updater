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
 * @file sd-helper.c
 * @author Lasse Mikkelsen <lkmi@prevas.dk>
 * @date 21 Sep 2018
 * @brief Systemd helper
 *
 */

#include "sd-helper.h"

/**
 * @brief Callback function: prepare GSource
 *
 * @param[in] source sd_event_source that should be prepared.
 * @param[in] timeout not used
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
static gboolean sd_source_prepare(GSource *source, gint *timeout)
{
        return sd_event_prepare(((struct SDSource *) source)->event) > 0 ? TRUE : FALSE;
}

/**
 * @brief Callback function: check GSource
 *
 * @param[in] source sd_event_source that should be checked
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
static gboolean sd_source_check(GSource *source)
{
        return sd_event_wait(((struct SDSource *) source)->event, 0) > 0 ? TRUE : FALSE;
}

/**
 * @brief Callback function: dispatch
 *
 * @param[in] source sd_event_source that should be dispatched
 * @param[in] callback not used
 * @param[in] userdata not used
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
static gboolean sd_source_dispatch(GSource *source,
                                   GSourceFunc callback,
                                   gpointer userdata)
{
        return sd_event_dispatch(((struct SDSource *) source)->event) >= 0
               ? G_SOURCE_CONTINUE
               : G_SOURCE_REMOVE;
}

/**
 * @brief Callback function: finalize GSource
 *
 * @param[in] source sd_event_source that should be finalized
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
static void sd_source_finalize(GSource *source)
{
        sd_event_unref(((struct SDSource *) source)->event);
}

/**
 * @brief Callback function: when source exits
 *
 * @param[in] source sd_event_source that exits
 * @param[in] userdata the GMainLoop the source is attached to.
 * @return Always return 0
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GMainLoop
 */
static int sd_source_on_exit(sd_event_source *source, void *userdata)
{
        g_main_loop_quit(userdata);

        sd_event_source_set_enabled(source, FALSE);
        sd_event_source_unref(source);

        return 0;
}

/**
 * @brief Attach GSource to GMainLoop
 *
 * @param[in] source Glib GSource
 * @param[in] loop GMainLoop the GSource should be attached to.
 * @return 0 if success else != 0 if error
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GMainLoop
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
int sd_source_attach(GSource *source, GMainLoop *loop)
{
        g_source_set_name(source, "sd-event");
        g_source_add_poll(source, &((struct SDSource *) source)->pollfd);
        g_source_attach(source, g_main_loop_get_context(loop));

        return sd_event_add_exit(((struct SDSource *) source)->event,
                                 NULL,
                                 sd_source_on_exit,
                                 loop);
}

/**
 * @brief Create GSource from a sd_event
 *
 * @param[in] event Systemd event that should be converted to a Glib GSource
 * @return the newly-created GSource
 * @see https://www.freedesktop.org/software/systemd/man/sd-event.html
 * @see https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#GSource
 */
GSource * sd_source_new(sd_event *event)
{
        static GSourceFuncs funcs = {
                sd_source_prepare,
                sd_source_check,
                sd_source_dispatch,
                sd_source_finalize,
        };
        GSource *s = g_source_new(&funcs, sizeof(struct SDSource));
        if (s) {
                ((struct SDSource *) s)->event = sd_event_ref(event);
                ((struct SDSource *) s)->pollfd.fd = sd_event_get_fd(event);
                ((struct SDSource *) s)->pollfd.events = G_IO_IN | G_IO_HUP;
        }

        return s;
}
