#ifndef __JSMN_UTILS_H
#define __JSMN_UTILS_H

#include <claud/jsmn.h>

#ifdef __cplusplus
}
#endif

size_t get_json_element_count(jsmntok_t *t);
jsmntok_t *find_json_element_by_name(const char *js, jsmntok_t *t,
		 size_t count, jsmntype_t type, const char *name);
int64_t parse_json_int64(const char *js, jsmntok_t *t);
char *parse_json_string(const char *js, jsmntok_t *t);
jsmntok_t *parse_json(jsmn_parser *p, const char *buf, size_t buf_size,
		      size_t *count);
int64_t get_json_int64_by_name(const char *js, jsmntok_t *t,
			       size_t count, const char *name);
int get_json_int_by_name(const char *js, jsmntok_t *t,
			 size_t count, const char *name);
char *get_json_string_by_name(const char *js, jsmntok_t *t,
			      size_t count, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __JSMN_UTILS_H */
