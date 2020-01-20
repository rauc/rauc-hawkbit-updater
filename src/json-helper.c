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
 * @file json-helper.c
 * @author Lasse Mikkelsen <lkmi@prevas.dk>
 * @date 19 Sep 2018
 * @brief JSON helper functions
 *
 */

#include "json-helper.h"

gchar* json_get_string(JsonNode *json_node, const gchar *path)
{
        char *res_str = NULL;
        JsonNode *result = json_path_query(path, json_node, NULL);
        if (result) {
                JsonArray *results = json_node_get_array(result);
                if (json_array_get_length( results ) > 0) {
                        res_str = g_strdup(json_array_get_string_element(results, 0));
                }
        }
        json_node_unref(result);
        return res_str;
}

gint64 json_get_int(JsonNode *json_node, const gchar *path)
{
        gint64 res_int = -1;
        JsonNode *result = json_path_query(path, json_node, NULL);
        if (result) {
                JsonArray *results = json_node_get_array(result);
                if (json_array_get_length( results ) > 0) {
                        res_int = json_array_get_int_element(results, 0);
                }
        }
        json_node_unref(result);
        return res_int;
}

JsonArray* json_get_array(JsonNode *json_node, const gchar *path)
{
        JsonArray *res_arr = NULL;
        JsonNode *result = json_path_query(path, json_node, NULL);
        if (result) {
                JsonArray *arr = json_node_get_array(result);
                if (json_array_get_length(arr) > 0) {
                        JsonNode *node = json_array_get_element(arr, 0);
                        if (JSON_NODE_HOLDS_ARRAY(node)) {
                                //g_debug("json_get_array: %s\n", json_to_string(result, TRUE));
                                res_arr = json_node_get_array(node);
                        }
                }
        }
        json_node_unref(result);
        return res_arr;
}

gboolean json_contains(JsonNode *root, gchar *key)
{
        JsonNode *node = json_path_query(key, root, NULL);
        gboolean result = (node != NULL && json_array_get_length(json_node_get_array(node)) > 0);
        json_node_unref(node);
        return result;
}
