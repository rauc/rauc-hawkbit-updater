/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 */

#ifndef __JSON_HELPER_H__
#define __JSON_HELPER_H__

#include <glib.h>
#include <glib/gtypes.h>
#include <json-glib/json-glib.h>

/**
 * @brief Get the string inside the first JsonNode element matching path in json_node.
 *
 * @param[in]  json_node JsonNode to evaluate expression on
 * @param[in]  path      JSONPath expression
 * @param[out] error     Error
 * @return gchar*, string value (must be freed), NULL on error (error set)
 */
gchar* json_get_string(JsonNode *json_node, const gchar *path, GError **error);

/**
 * @brief Get the integer inside the first JsonNode element matching path in json_node.
 *
 * @param[in]  json_node JsonNode to evaluate expression on
 * @param[in]  path      JSONPath expression
 * @param[out] error     Error
 * @return gint64, integer value, 0 on error (error set)
 */
gint64 json_get_int(JsonNode *json_node, const gchar *path, GError **error);

/**
 * @brief Get the JsonArray inside the first JsonNode element matching path in json_node.
 *
 * @param[in]  json_node JsonNode to evaluate expression on
 * @param[in]  path      JSONPath expression
 * @param[out] error     Error
 * @return JsonArray*, array (must be freed), NULL on error (error set)
 */
JsonArray* json_get_array(JsonNode *json_node, const gchar *path, GError **error);

/**
 * @brief Check if the given path matches an element in json_node.
 *
 * @param[in] json_node JsonNode to evaluate expression on
 * @param[in] path      JSONPath expression
 * @return gboolean, TRUE if path matches an element, FALSE otherwise
 */
gboolean json_contains(JsonNode *root, gchar *key);

#endif // __JSON_HELPER_H__
