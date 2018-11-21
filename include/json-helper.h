#ifndef __JSON_HELPER_H__
#define __JSON_HELPER_H__

#include <glib-2.0/glib.h>
#include <json-glib/json-glib.h>

gchar* json_get_string(JsonNode *json_node, const gchar *path);
const gint64 json_get_int(JsonNode *json_node, const gchar *path);
JsonArray* json_get_array(JsonNode *json_node, const gchar *path);
gboolean json_contains(JsonNode *root, gchar *key);

#endif // __JSON_HELPER_H__
