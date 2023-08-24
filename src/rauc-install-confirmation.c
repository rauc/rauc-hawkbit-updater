/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2023 Vyacheslav Yurkov <uvv.mail@gmail.com>
 *
 * @file
 * @brief Confirmation request client
 */

#include <gio/gio.h>
#include <glib-object.h>
#include <glib/gtypes.h>
#include <stdio.h>
#include "gobject/gclosure.h"
#include "rauc-install-confirmation.h"
#include "rauc-install-confirmation-gen.h"

static GThread *thread_confirmation = NULL;

/**
 * @brief Confirmation DBUS signal callback
 */
static void on_confirmation_received(GDBusProxy *proxy, gint action_id, gboolean confirmed,
                                     gint error_code, gchar *details, gpointer data)
{
        struct confirm_context *context = data;

        g_return_if_fail(context);

        context->confirmed = confirmed;
        context->error_code = error_code;
        context->details = g_strdup(details);
        g_main_loop_quit(context->mainloop);
}

/**
 * @brief Create and init a confirm_context
 *
 * @return Pointer to initialized confirm_context struct. Should be freed by calling
 *         confirm_context_free().
 */
static struct confirm_context *confirm_context_new(void)
{
        struct confirm_context *context = g_new0(struct confirm_context, 1);

        g_mutex_init(&context->status_mutex);

        return context;
}

/**
 * @brief Free a confirm_context and its members
 *
 * @param[in] context the confirm_context struct that should be freed.
 *            If NULL
 */
static void confirm_context_free(struct confirm_context *context)
{
        if (!context)
                return;

        g_free(context->action_id);
        g_free(context->new_version);
        g_free(context->details);

        g_mutex_clear(&context->status_mutex);

        // make sure all pending events are processed
        while (g_main_context_iteration(context->loop_context, FALSE));
        g_main_context_unref(context->loop_context);

        g_main_loop_unref(context->mainloop);
        g_free(context);
}

/**
 * @brief Confirmation request mainloop
 *
 * Install mainloop running until confirmation response is received.
 * @param[in] data pointer to a confirm_context struct.
 * @return NULL is always returned.
 */
static gpointer confirmation_loop_thread(gpointer data)
{
        GBusType bus_type = (!g_strcmp0(g_getenv("DBUS_STARTER_BUS_TYPE"), "session"))
                            ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;
        RInstallConfirmation *r_confirmation_proxy = NULL;
        g_autoptr(GError) error = NULL;
        struct confirm_context *context = NULL;

        g_return_val_if_fail(data, NULL);

        context = data;
        g_main_context_push_thread_default(context->loop_context);

        g_debug("Creating Confirmation DBUS proxy");
        r_confirmation_proxy = r_install_confirmation_proxy_new_for_bus_sync(
                bus_type, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                "de.pengutronix.rauc.InstallConfirmation", "/", NULL, &error);
        if (!r_confirmation_proxy) {
                g_warning("Failed to create confirmation DBUS proxy: %s", error->message);
                goto notify_complete;
        }
        if (g_signal_connect(r_confirmation_proxy, "confirmation-status",
                             G_CALLBACK(on_confirmation_received), context) <= 0) {
                g_warning("Failed to connect completed signal");
                goto out_loop;
        }

        g_debug("Asking to confirm installation over DBus");
        if (!r_install_confirmation_call_confirm_installation_request_sync(r_confirmation_proxy,
                                                                           context->action_id,
                                                                           context->new_version,
                                                                           NULL, &error)) {
                g_warning("%s", error->message);
                goto out_loop;
        }

        g_main_loop_run(context->mainloop);

out_loop:
        g_signal_handlers_disconnect_by_data(r_confirmation_proxy, context);

notify_complete:
        // Notify the result of the confirmation
        context->notify_confirm(context);

        g_clear_pointer(&r_confirmation_proxy, g_object_unref);
        g_main_context_pop_thread_default(context->loop_context);

        confirm_context_free(context);
        return NULL;
}

void rauc_installation_confirm(const gchar *action_id, const gchar *version, GSourceFunc on_confirm)
{
        GMainContext *loop_context = NULL;
        struct confirm_context *context = NULL;

        loop_context = g_main_context_new();
        context = confirm_context_new();

        context->notify_confirm = on_confirm;

        context->mainloop = g_main_loop_new(loop_context, FALSE);
        context->loop_context = loop_context;
        context->action_id = g_strdup(action_id);
        context->new_version = g_strdup(version);

        if (thread_confirmation)
                g_thread_join(thread_confirmation);

        thread_confirmation = g_thread_new("install-confirmation", confirmation_loop_thread, (gpointer) context);
}
