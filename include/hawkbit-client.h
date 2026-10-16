/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 */

#ifndef __HAWKBIT_CLIENT_H__
#define __HAWKBIT_CLIENT_H__

#include <glib.h>
#include <glib/gtypes.h>
#include <stdio.h>

#include "config-file.h"

#define RHU_HAWKBIT_CLIENT_ERROR rhu_hawkbit_client_error_quark()
GQuark rhu_hawkbit_client_error_quark(void);

typedef enum {
        RHU_HAWKBIT_CLIENT_ERROR_ALREADY_IN_PROGRESS,
        RHU_HAWKBIT_CLIENT_ERROR_JSON_RESPONSE_PARSE,
        RHU_HAWKBIT_CLIENT_ERROR_MULTI_CHUNKS,
        RHU_HAWKBIT_CLIENT_ERROR_MULTI_ARTIFACTS,
        RHU_HAWKBIT_CLIENT_ERROR_DOWNLOAD,
        RHU_HAWKBIT_CLIENT_ERROR_STREAM_INSTALL,
        RHU_HAWKBIT_CLIENT_ERROR_CANCELATION,
} RHUHawkbitClientError;

// uses CURLcode as error codes
#define RHU_HAWKBIT_CLIENT_CURL_ERROR rhu_hawkbit_client_curl_error_quark()
GQuark rhu_hawkbit_client_curl_error_quark(void);

// uses HTTP codes as error codes
#define RHU_HAWKBIT_CLIENT_HTTP_ERROR rhu_hawkbit_client_http_error_quark()
GQuark rhu_hawkbit_client_http_error_quark(void);

#define HAWKBIT_USERAGENT                 "rauc-hawkbit-c-agent/1.0"
#define DEFAULT_CURL_REQUEST_BUFFER_SIZE  512
#define DEFAULT_CURL_DOWNLOAD_BUFFER_SIZE 64 * 1024 // 64KB

extern gboolean run_once;                  /**< only run software check once and exit */

/**
 * @brief HTTP methods.
 */
enum HTTPMethod {
        GET,
        HEAD,
        PUT,
        POST,
        PATCH,
        DELETE
};

enum ActionState {
        ACTION_STATE_NONE,
        ACTION_STATE_CANCELED,
        ACTION_STATE_ERROR,
        ACTION_STATE_SUCCESS,
        ACTION_STATE_PROCESSING,
        ACTION_STATE_DOWNLOADING,
        ACTION_STATE_INSTALLING,
        ACTION_STATE_CANCEL_REQUESTED,
};

enum ConfirmationState {
        CONFIRMATION_STATE_NONE,
        CONFIRMATION_STATE_REQUESTED,
        CONFIRMATION_STATE_CONFIRMED,
        CONFIRMATION_STATE_DENIED,
};

/**
 * @brief struct that contains the context of an HawkBit action.
 */
struct HawkbitAction {
        gchar *id;                    /**< HawkBit action id */
        GMutex mutex;                 /**< mutex used for accessing all other members */
        enum ActionState state;       /**< state of this action */
        GCond cond;                   /**< condition on state */
};

/**
 * @brief struct that contains the context of a confirmation action.
 */
struct ConfirmationAction {
        gchar *id;                    /**< HawkBit action id */
        GMutex mutex;                 /**< mutex used for synchronization of the state */
        GCond cond;                   /**< condition on confirmation */
        enum ConfirmationState state; /**< current confirmation state */
};

/**
 * @brief struct containing the payload and size of REST body.
 */
typedef struct RestPayload_ {
        gchar *payload;               /**< string representation of payload */
        size_t size;                  /**< size of payload */
} RestPayload;

/**
 * @brief struct containing data about an artifact that is currently being deployed.
 */
typedef struct Artifact_ {
        gchar *name;                  /**< name of software */
        gchar *version;               /**< software version */
        gint64 size;                  /**< size of software bundle file */
        gchar *download_url;          /**< download URL of software bundle file */
        gchar *feedback_url;          /**< URL status feedback should be sent to */
        gchar *sha1;                  /**< sha1 checksum of software bundle file */
        gchar *maintenance_window;    /**< maintenance flag, possible values: available, unavailable, null */
        gboolean do_install;          /**< whether the installation should be started or not */
} Artifact;

/**
 * @brief struct used to store active confirmation info
 */
typedef struct Confirmation_ {
        gchar *action_id;              /**< Hawkbit's ID of the request */
        gchar *version;                /**< software version */
} Confirmation;

/**
 * @brief struct containing the new downloaded file.
 */
struct on_new_software_userdata {
        GSourceFunc install_progress_callback;  /**< callback function to be called when new progress */
        GSourceFunc install_complete_callback;  /**< callback function to be called when installation is complete */
        gchar *file;                            /**< downloaded new software file */
        gchar *auth_header;                     /**< authentication header for bundle streaming */
        gchar *ssl_key;                         /**< authentication key for bundle streaming */
        gchar *ssl_cert;                        /**< authentication certificate for bundle streaming */
        gboolean ssl_verify;                    /**< whether to ignore server cert verification errors */
        gboolean install_success;               /**< whether the installation succeeded or not (only meaningful for run_once mode!) */
};

/**
 * @brief struct containing the result of the installation.
 */
struct on_install_complete_userdata {
        gboolean install_success;               /**< status of installation */
};

/**
 * @brief struct containing a confirmation request.
 */
struct on_install_confirmation_request_userdata {
        GSourceFunc response_callback; /**< callback function to be called when response is received */
        gchar *action_id;              /**< Hawkbit's ID of the request */
        gchar *version;                /**< software version */
};

/**
 * @brief struct containing a confirmation response from a user software
 */
struct on_install_confirmed_userdata {
        gchar *action_id;              /**< Hawkbit's ID of the request */
        gboolean confirmed;            /**< True - confirmed, False - denied */
        gchar *details;                /**< Explanation about confirmation status (if any) */
        gint error_code;               /**< Code to be returned to Hawkbit */
};

/**
 * @brief Pass config, callback for installation ready and initialize libcurl.
 *        Intended to be called from program's main().
 *
 * @param[in] config Config* to make global
 * @param[in] on_install_ready GSourceFunc to call after artifact download, to
 *                             trigger RAUC installation
 * @param[in] on_install_confirm GSourceFunc to call when confirmation status
 *                             is received from a user
 */
void hawkbit_init(Config *config, GSourceFunc on_install_ready,
                GSourceFunc on_install_confirmed);

/**
 * @brief Sets up timeout and event sources, initializes and runs main loop.
 *
 * @return numeric return code, to be returned by main()
 */
int hawkbit_start_service_sync();

/**
 * @brief Callback for install thread, sends msg as progress feedback to
 *        hawkBit.
 *
 * @param[in] msg Progress message
 * @return G_SOURCE_REMOVE is always returned
 */
gboolean hawkbit_progress(const gchar *msg);

/**
 * @brief Callback for install thread, sends installation feedback to hawkBit.
 *
 * @param[in] ptr on_install_complete_userdata* containing set install_success
 * @return G_SOURCE_REMOVE is always returned
 */
gboolean install_complete_cb(gpointer ptr);

/**
 * @brief Frees the memory allocated by a RestPayload
 *
 * @param[in] payload RestPayload to free
 */
void rest_payload_free(RestPayload *payload);

/**
 * @brief Frees the memory allocated by an Artifact
 *
 * @param[in] artifact Artifact to free
 */
void artifact_free(Artifact *artifact);

/**
 * @brief Frees the memory allocated by a Confirmation
 *
 * @param[in] confirmation Confirmation to free
 */
void confirmation_free(Confirmation *confirmation);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(RestPayload, rest_payload_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(Artifact, artifact_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(Confirmation, confirmation_free)

#endif // __HAWKBIT_CLIENT_H__
