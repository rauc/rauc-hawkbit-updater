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

#ifndef __JSON_HELPER_H__
#define __JSON_HELPER_H__

#include <glib-2.0/glib.h>
#include <json-glib/json-glib.h>

gchar* json_get_string(JsonNode *json_node, const gchar *path);
gint64 json_get_int(JsonNode *json_node, const gchar *path);
JsonArray* json_get_array(JsonNode *json_node, const gchar *path);
gboolean json_contains(JsonNode *root, gchar *key);

#endif // __JSON_HELPER_H__
