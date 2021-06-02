/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 *
 * @file
 * @brief JSON helper functions
 */

#include "json-helper.h"
#include <stddef.h>


/**
 * @brief Get the first JsonNode element matching path in json_node.
 *
 * @param[in]  json_node JsonNode to query
 * @param[in]  path      Query path
 * @param[out] error     Error
 * @return JsonNode*, matching JsonNode element (must be freed), NULL on error
 */
static JsonNode* json_get_first_matching_element(JsonNode *json_node, const gchar *path,
                                                 GError **error)
{
        g_autoptr(JsonNode) match = NULL, node = NULL;
        JsonArray *arr = NULL;

        g_return_val_if_fail(json_node, NULL);
        g_return_val_if_fail(path, NULL);
        g_return_val_if_fail(error == NULL || *error == NULL, NULL);

        match = json_path_query(path, json_node, error);
        if (!match)
                return NULL;

        arr = json_node_get_array(match);
        if (!arr) {
                g_set_error(error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_PARSE,
                            "Failed to retrieve array from node for path %s", path);
                return NULL;
        }

        if (json_array_get_length(arr) > 0)
                node = json_array_dup_element(arr, 0);

        if (!node) {
                g_set_error(error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_PARSE,
                            "Failed to retrieve element from array for path %s", path);
                return NULL;
        }

        return g_steal_pointer(&node);
}

gchar* json_get_string(JsonNode *json_node, const gchar *path, GError **error)
{
        g_autofree gchar *res_str = NULL;
        g_autoptr(JsonNode) result = NULL;

        g_return_val_if_fail(json_node, NULL);
        g_return_val_if_fail(path, NULL);
        g_return_val_if_fail(error == NULL || *error == NULL, NULL);

        result = json_get_first_matching_element(json_node, path, error);
        if (!result)
                return NULL;

        res_str = json_node_dup_string(result);
        if (!res_str) {
                g_set_error(error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_PARSE,
                            "Failed to retrieve string element from array for path %s", path);
                return NULL;
        }

        return g_steal_pointer(&res_str);
}

gint64 json_get_int(JsonNode *json_node, const gchar *path, GError **error)
{
        g_autoptr(JsonNode) result = NULL;

        g_return_val_if_fail(json_node, 0);
        g_return_val_if_fail(path, 0);
        g_return_val_if_fail(error == NULL || *error == NULL, 0);

        result = json_get_first_matching_element(json_node, path, error);
        if (!result)
                return 0;

        if (!JSON_NODE_HOLDS_VALUE(result)) {
                g_set_error(error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_PARSE,
                            "Failed to retrieve value from node for path %s", path);
                return 0;
        }

        return json_node_get_int(result);
}

JsonArray* json_get_array(JsonNode *json_node, const gchar *path, GError **error)
{
        g_autoptr(JsonArray) res_arr = NULL;
        g_autoptr(JsonNode) result = NULL;

        g_return_val_if_fail(error == NULL || *error == NULL, NULL);
        g_return_val_if_fail(json_node, NULL);
        g_return_val_if_fail(path, NULL);

        result = json_get_first_matching_element(json_node, path, error);
        if (!result)
                return NULL;

        if (!JSON_NODE_HOLDS_ARRAY(result)) {
                g_set_error(error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_PARSE,
                            "Failed to retrieve value from node for path %s", path);
                return NULL;
        }

        res_arr = json_node_dup_array(result);
        if (!res_arr || !json_array_get_length(res_arr)) {
                g_set_error(error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_PARSE,
                            "Empty JSON array for path %s", path);
                return NULL;
        }

        return g_steal_pointer(&res_arr);
}

gboolean json_contains(JsonNode *json_node, gchar *path)
{
        g_autoptr(GError) error = NULL;
        g_autoptr(JsonNode) node = NULL;

        g_return_val_if_fail(json_node, FALSE);
        g_return_val_if_fail(path, FALSE);

        node = json_path_query(path, json_node, &error);
        if (!node) {
                // failed to compile expression to JSONPath
                g_warning("%s", error->message);
                return FALSE;
        }

        if (json_array_get_length(json_node_get_array(node)) > 0)
                return TRUE;

        return FALSE;
}
