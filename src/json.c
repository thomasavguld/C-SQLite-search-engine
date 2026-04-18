#include "json.h"
#include "yyjson.h"
#include <string.h>
#include <stdlib.h>


char *extract_title(const char *json_str) {
	yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
	if (!doc) return strdup("");

	yyjson_val *root = yyjson_doc_get_root(doc);
	if (!root) {
		yyjson_doc_free(doc);
		return strdup("");
	}

	yyjson_val *title = yyjson_obj_get(root, "title");

	char *out = strdup(
		(title && yyjson_is_str(title)) ? yyjson_get_str(title) : ""
	);

	yyjson_doc_free(doc);
	return out;
	}

char *extract_abstract(const char *json_str) {
	yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
	if (!doc) return strdup("");

	yyjson_val *root = yyjson_doc_get_root(doc);
	if (!root) {
		yyjson_doc_free(doc);
		return strdup("");
	}

	yyjson_val *abstract = yyjson_obj_get(root, "abstract");

	char *out = strdup(
		(abstract && yyjson_is_str(abstract)) ? yyjson_get_str(abstract) : ""
	);

	yyjson_doc_free(doc);
	return out;
	
}
