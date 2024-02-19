/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2021-2022 Bastian Krause <bst@pengutronix.de>, Pengutronix
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 *
 * @file
 * @brief RAUC client
 */

#include <gio/gio.h>
#include <glib-object.h>
#include <glib/gtypes.h>
#include <stdio.h>
#include "gobject/gclosure.h"
#include "rauc-installer.h"
#include "rauc-installer-gen.h"

static GThread *thread_install = NULL;

/**
 * @brief RAUC DBUS property changed callback
 *
 * @see https://github.com/rauc/rauc/blob/master/src/de.pengutronix.rauc.Installer.xml
 */
static void on_installer_status(GDBusProxy *proxy, GVariant *changed,
                                const gchar* const *invalidated, gpointer data)
{
        struct install_context *context = data;
        gint32 percentage;
        g_autofree gchar *message = NULL;

        g_return_if_fail(changed);
        g_return_if_fail(context);

        if (invalidated && invalidated[0]) {
                g_warning("RAUC DBUS service disappeared");
                g_mutex_lock(&context->status_mutex);
                context->status_result = 2;
                g_mutex_unlock(&context->status_mutex);
                g_main_loop_quit(context->mainloop);
                return;
        }

        if (context->notify_event) {
                gboolean status_received = FALSE;

                g_mutex_lock(&context->status_mutex);
                if (g_variant_lookup(changed, "Operation", "s", &message))
                        g_queue_push_tail(&context->status_messages, g_steal_pointer(&message));
                else if (g_variant_lookup(changed, "Progress", "(isi)", &percentage, &message,
                                          NULL))
                        g_queue_push_tail(&context->status_messages,
                                          g_strdup_printf("%3" G_GINT32_FORMAT "%% %s", percentage,
                                                          message));
                else if (g_variant_lookup(changed, "LastError", "s", &message) && message[0] != 0)
                        g_queue_push_tail(&context->status_messages,
                                          g_strdup_printf("LastError: %s", message));

                status_received = !g_queue_is_empty(&context->status_messages);
                g_mutex_unlock(&context->status_mutex);

                if (status_received)
                        g_main_context_invoke(context->loop_context, context->notify_event,
                                              context);
        }
}

/**
 * @brief RAUC DBUS complete signal callback
 *
 * @see https://github.com/rauc/rauc/blob/master/src/de.pengutronix.rauc.Installer.xml
 */
static void on_installer_completed(GDBusProxy *proxy, gint result, gpointer data)
{
        struct install_context *context = data;

        g_return_if_fail(context);

        g_mutex_lock(&context->status_mutex);
        context->status_result = result;
        g_mutex_unlock(&context->status_mutex);

        if (result >= 0)
                g_main_loop_quit(context->mainloop);
}

/**
 * @brief Create and init a install_context
 *
 * @return Pointer to initialized install_context struct. Should be freed by calling
 *         install_context_free().
 */
static struct install_context *install_context_new(void)
{
        struct install_context *context = g_new0(struct install_context, 1);

        g_mutex_init(&context->status_mutex);
        g_queue_init(&context->status_messages);
        context->status_result = -2;

        return context;
}

/**
 * @brief Free a install_context and its members
 *
 * @param[in] context the install_context struct that should be freed.
 *                    If NULL
 */
static void install_context_free(struct install_context *context)
{
        if (!context)
                return;

        g_free(context->bundle);
        g_free(context->auth_header);
        g_mutex_clear(&context->status_mutex);

        // make sure all pending events are processed
        while (g_main_context_iteration(context->loop_context, FALSE));
        g_main_context_unref(context->loop_context);

        g_assert_cmpint(context->status_result, >=, 0);
        g_assert_true(g_queue_is_empty(&context->status_messages));
        g_main_loop_unref(context->mainloop);
        g_free(context);
}

/**
 * @brief RAUC client mainloop
 *
 * Install mainloop running until installation completes.
 * @param[in] data pointer to a install_context struct.
 * @return NULL is always returned.
 */
static gpointer install_loop_thread(gpointer data)
{
        GBusType bus_type = (!g_strcmp0(g_getenv("DBUS_STARTER_BUS_TYPE"), "session"))
                            ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;
        RInstaller *r_installer_proxy = NULL;
        g_autoptr(GError) error = NULL;
        g_auto(GVariantDict) args = G_VARIANT_DICT_INIT(NULL);
        struct install_context *context = NULL;

        g_return_val_if_fail(data, NULL);

        context = data;
        g_main_context_push_thread_default(context->loop_context);

        if (context->auth_header) {
                gchar *headers[2] = {NULL, NULL};
                headers[0] = context->auth_header;
                g_variant_dict_insert(&args, "http-headers", "^as", headers);
                g_variant_dict_insert(&args, "tls-no-verify", "b", !context->ssl_verify);
        }
        if (context->ssl_key && context->ssl_cert) {
                g_variant_dict_insert(&args, "tls-key", "s", context->ssl_key);
                g_variant_dict_insert(&args, "tls-cert", "s", context->ssl_cert);
                g_variant_dict_insert(&args, "tls-no-verify", "b", !context->ssl_verify);
        }

        g_debug("Creating RAUC DBUS proxy");
        r_installer_proxy = r_installer_proxy_new_for_bus_sync(
                bus_type, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                "de.pengutronix.rauc", "/", NULL, &error);
        if (!r_installer_proxy) {
                g_warning("Failed to create RAUC DBUS proxy: %s", error->message);
                goto notify_complete;
        }
        if (g_signal_connect(r_installer_proxy, "g-properties-changed",
                             G_CALLBACK(on_installer_status), context) <= 0) {
                g_warning("Failed to connect properties-changed signal");
                goto out_loop;
        }
        if (g_signal_connect(r_installer_proxy, "completed",
                             G_CALLBACK(on_installer_completed), context) <= 0) {
                g_warning("Failed to connect completed signal");
                goto out_loop;
        }

        g_debug("Trying to contact RAUC DBUS service");
        if (!r_installer_call_install_bundle_sync(r_installer_proxy, context->bundle,
                                                  g_variant_dict_end(&args), NULL, &error)) {
                g_warning("%s", error->message);
                goto out_loop;
        }

        g_main_loop_run(context->mainloop);

out_loop:
        g_signal_handlers_disconnect_by_data(r_installer_proxy, context);

notify_complete:
        // Notify the result of the RAUC installation
        if (context->notify_complete)
                context->notify_complete(context);

        g_clear_pointer(&r_installer_proxy, g_object_unref);
        g_main_context_pop_thread_default(context->loop_context);

        // on wait, calling function will take care of freeing after reading context->status_result
        if (!context->keep_install_context)
                install_context_free(context);
        return NULL;
}

gboolean rauc_install(const gchar *bundle, const gchar *auth_header,
                      gchar *ssl_key, gchar *ssl_cert, gboolean ssl_verify,
                      GSourceFunc on_install_notify, GSourceFunc on_install_complete,
                      gboolean wait)
{
        GMainContext *loop_context = NULL;
        struct install_context *context = NULL;

        g_return_val_if_fail(bundle, FALSE);

        loop_context = g_main_context_new();
        context = install_context_new();
        context->bundle = g_strdup(bundle);
        context->auth_header = g_strdup(auth_header);
        context->ssl_key = ssl_key,
        context->ssl_cert = ssl_cert,
        context->ssl_verify = ssl_verify;
        context->notify_event = on_install_notify;
        context->notify_complete = on_install_complete;
        context->mainloop = g_main_loop_new(loop_context, FALSE);
        context->loop_context = loop_context;
        context->status_result = 2;
        context->keep_install_context = wait;

        // unref/free previous install thread by joining it
        if (thread_install)
                g_thread_join(thread_install);

        // start install thread
        thread_install = g_thread_new("installer", install_loop_thread, (gpointer) context);
        if (wait) {
                gboolean result;

                g_thread_join(thread_install);
                result = context->status_result == 0;

                install_context_free(context);
                return result;
        }

        // return immediately if we did not wait for the install thread
        return TRUE;
}
