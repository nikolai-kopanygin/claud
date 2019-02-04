#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <types.h>
#include <malloc.h>

#include "jsmn_utils.h"
#include "utils.h"

/* Function realloc_it() is a wrapper function for standard realloc()
 * with one difference - it frees old memory pointer in case of realloc
 * failure. Thus, DO NOT use old data pointer in anyway after call to
 * realloc_it(). If your code has some kind of fallback algorithm if
 * memory can't be re-allocated - use standard realloc() instead.
 */
static inline void *realloc_it(void *ptrmem, size_t size) {
	void *p = realloc(ptrmem, size);
	if (!p)  {
		free (ptrmem);
		log_error("realloc(): errno=%d\n", errno);
	}
	return p;
}

/**
 * This function counts the nodes of a JSON token tree.
 * @param t - the root token.
 * @return the number of the nodes
 */
size_t get_json_element_count(jsmntok_t *t)
{
	int i, j;
	if (t->type == JSMN_PRIMITIVE) {
		return 1;
	} else if (t->type == JSMN_STRING) {
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		j = 0;
		for (i = 0; i < t->size; i++) {
			j += get_json_element_count(t+1+j); // keys
			j += get_json_element_count(t+1+j); // values
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		for (i = 0; i < t->size; i++) {
			j += get_json_element_count(t+1+j);
		}
		return j+1;
	}
	return 0;
}

/**
 * Find by name a JSON token of a specified type in a JSON token tree.
 * @param js - the JSON code buffer;
 * @param t - pointer to the root element of the JSON token tree;
 * @param count - maximum number of tokens in the tree;
 * @param type - the token type;
 * @param name - the token name.
 * @return a pointer to the token, or NULL if not found.
 */
jsmntok_t *find_json_element_by_name(const char *js, jsmntok_t *t,
		 size_t count, jsmntype_t type, const char *name)
{
	size_t i;
	for (i = 0; i < count - 1; i++) {
		if (type != t[i+1].type)
			continue;
		if (JSMN_STRING != t[i].type)
			continue;
		if (strncmp(name, js + t[i].start,
			    t[i].end - t[i].start) == 0)
			break;
	}
	if (i == count - 1)
		return NULL;

	return &t[i+1];
}

int parse_json_int(const char *js, jsmntok_t *t)
{
	int val = 0;
	size_t len;
	char *buf;

	if (!js || !t)
		return 0;

	len = (size_t)(t->end - t->start);
	
	buf = malloc(len+1);
	if (!buf) {
		log_error("Failed to alloc memory\n");
		return 0;
	}
	strncpy(buf, js + t->start, len);
	buf[len] = '\0';
	if (sscanf(buf, "%d", &val) <= 0) {
		log_error("Could not parse int value\n");
		val = 0;
	}
	free(buf);
	return val;
}

int64_t parse_json_int64(const char *js, jsmntok_t *t)
{
	int64_t val = 0;
	size_t len;
	char *buf;
	
	if (!js || !t)
		return 0;

	len = (size_t)(t->end - t->start);
	
	buf = malloc(len+1);
	if (!buf) {
		log_error("Failed to alloc memory\n");
		return 0;
	}
	
	strncpy(buf, js + t->start, len);
	buf[len] = '\0';
	if (sscanf(buf, "%ld", &val) <= 0) {
		log_error("Could not parse 64-bit int value\n");
		val = 0;
	}
	free(buf);
	return val;
}

char *parse_json_string(const char *js, jsmntok_t *t)
{
	if (!js || !t)
		return NULL;
	
	return strndup(js + t->start, (t->end - t->start));
}

/**
 * Allocate a JSON token array, parse the memory buffer into tokens and
 * put them into the token array. Return a pointer to the token array and
 * the number of parsed tokens.
 * @param p - the parser;
 * @param buf - the memory buffer containing JSON code;
 * @param buf_size - the size of the memory buffer;
 * @param count - the pointer used to return the number of parsed tokens;
 * @return a pointer to the JSON token array.
 */
jsmntok_t *parse_json(jsmn_parser *p, const char *buf, size_t buf_size,
		      size_t *count)
{
	int r = 0;
	jsmntok_t *tok = NULL;
	size_t tokcount = 1;

	do {
		tokcount = tokcount * 2;
		tok = realloc_it(tok, sizeof(jsmntok_t) * tokcount);
		if (tok == NULL) {
			*count = 0;
			log_error("Could not allocate JSON tokens!\n");
			return NULL;
		}
		r = jsmn_parse(p, buf, buf_size, tok, tokcount);
	} while (r == JSMN_ERROR_NOMEM);
	
	if (r < 0) {
		free(tok);
		tokcount = 0;
	}
	
	*count = tokcount;
	return tok;
}

int64_t get_json_int64_by_name(const char *js, jsmntok_t *t,
			       size_t count, const char *name)
{
	return parse_json_int64(js,
		find_json_element_by_name(js, t, count, JSMN_PRIMITIVE, name));
}

int get_json_int_by_name(const char *js, jsmntok_t *t,
			 size_t count, const char *name)
{
	return parse_json_int(js,
		find_json_element_by_name(js, t, count, JSMN_PRIMITIVE, name));
}

char *get_json_string_by_name(const char *js, jsmntok_t *t,
			      size_t count, const char *name)
{
	return parse_json_string(js,
		find_json_element_by_name(js, t, count, JSMN_STRING, name));
}

