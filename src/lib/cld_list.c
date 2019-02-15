#include <time.h>
#include <curl/curl.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <claud/types.h>
#include <claud/http_api.h>
#include <claud/cld.h>
#include <claud/jsmn_utils.h>
#include <claud/utils.h>

/**
 * Parse the JSON-encoded directory entry description to
 * the specified list_item structure.
 * @param js - the JSON code;
 * @param tok - the JSON element of the directory entry;
 * @param count - the number of elements in the JSON subtree;
 * @param li - a pointer to the list_item struture.
 */
static void parse_list_item(const char *js, jsmntok_t *tok, size_t count,
			    struct list_item *li)
{
	li->mtime = (time_t)get_json_int64_by_name(js, tok, count, "mtime");
	li->kind = get_json_string_by_name(js, tok, count, "kind");
	li->size = get_json_int64_by_name(js, tok, count, "size");
	li->name = get_json_string_by_name(js, tok, count, "name");
	li->hash = get_json_string_by_name(js, tok, count, "hash");
}

/**
 * Parse the JSON-encoded contents of a mail.ru cloud directory to
 * the specified file_list structure.
 * @param js - the JSON code;
 * @param tok - the root JSON element;
 * @param finfo - a pointer to the file_list struture.
 * @return 0 for success, or error code.
 */
static int parse_file_list(const char *js, jsmntok_t *tok,
			   struct file_list *finfo)
{
	jsmntok_t *body, *list, *t;
	size_t count, body_count;
	size_t i;
	
	count = get_json_element_count(tok);
	body = find_json_element_by_name(js, tok, count, JSMN_OBJECT, "body");
	if (!body) {
		log_error("Wrongly formatted file list info: no body\n");
		return 1;
	}
	body_count = get_json_element_count(body);
	list = find_json_element_by_name(js, tok, count, JSMN_ARRAY, "list");
	if (!list) {
		log_error("Wrongly formatted file list info: no list\n");
		return 1;
	}
	finfo->body.list = calloc(list->size, sizeof(struct list_item));
	if (!finfo->body.list) {
		log_error("Failed to allocate a list of %d elements\n",
			list->size);
		return 1;
	}
	
	finfo->body.nr_list_items = list->size;
	t = list + 1;
	for (i = 0; i < list->size; i++) {
		size_t count = get_json_element_count(t);
		parse_list_item(js, t, count, &finfo->body.list[i]);
		t += count;
	}
	
	return 0;
}

/**
 * Read the contents of a mail.ru cloud directory to the specified
 * file_list structure.
 * @param c - the cloud descriptor;
 * @param path - the directory path;
 * @param finfo - a pointer to the file_list structure.
 * @result 0 for success, or error code.
 */
int cld_get_file_list(struct cld *c, const char *path, struct file_list *finfo)
{
	int res;
	jsmn_parser p;
	jsmntok_t *tok = NULL;
	size_t tokcount = 0;
	struct memory_struct chunk;

	const char *p_names[] = { "home", "token" };
	const char *p_values[] = { path, c->auth_token };
	char *url = make_url_with_params(c->curl, "folder", p_names, p_values, ARRAY_SIZE(p_names));
	
	if (!url)
		return 1;

	memory_struct_init(&chunk);
	res = get_req(c->curl, &chunk, url);
	free(url);
	
	if (res) {
		log_error("Get failed\n");
		return res;
	}

	/* Prepare parser */
	jsmn_init(&p);

	tok = parse_json(&p, chunk.memory, chunk.size, &tokcount);
	if (!tok) {
		printf("Could not parse JSON\n");
		chunk.memory[chunk.size - 1] = 0;
		printf("%s\n", chunk.memory);
		res = 1;
	} else {
		res = parse_file_list(chunk.memory, tok, finfo);
	}

	free(tok);
	memory_struct_cleanup(&chunk);
	return res;
}

/**
 * Clean up a list_item structure.
 * @param li - a pointer to the list_item struture.
 */
static void list_item_cleanup(struct list_item *li)
{
	free(li->kind);
	free(li->hash);
	free(li->name);
}

/**
 * Clean up a file_list structure.
 * @param finfo - a pointer to the file_list struture.
 */
void cld_file_list_cleanup(struct file_list *finfo)
{
	size_t i;
	if (!finfo)
		return;
	free(finfo->body.kind);
	free(finfo->body.hash);
	free(finfo->body.name);
	for (i = 0; i < finfo->body.nr_list_items; i++)
		list_item_cleanup(&finfo->body.list[i]);
	free(finfo->body.list);
	memset(finfo, 0, sizeof(*finfo));
}

/**
 * Parse the JSON-encoded file or directory info to
 * the specified file_list structure.
 * @param js - the JSON code;
 * @param tok - the root JSON element;
 * @param finfo - a pointer to the file_list struture.
 * @return 0 for success, or error code.
 */
static int parse_file_stat(const char *js, jsmntok_t *tok, struct file_list *finfo)
{
	jsmntok_t *body;
	size_t count, body_count;
	
	count = get_json_element_count(tok);
	body = find_json_element_by_name(js, tok, count, JSMN_OBJECT, "body");
	if (!body) {
		log_error("Wrongly formatted file info\n");
		return 1;
	}
	body_count = get_json_element_count(body);
	finfo->body.mtime = (time_t)get_json_int64_by_name(js, body, body_count, "mtime");
	finfo->body.kind = get_json_string_by_name(js, tok, count, "kind");
	finfo->body.size = get_json_int64_by_name(js, body, body_count, "size");
	finfo->body.name = get_json_string_by_name(js, tok, count, "name");
	finfo->body.hash = get_json_string_by_name(js, tok, count, "hash");
	return 0;
}

/**
 * Read the file or directory info to the specified
 * file_list structure.
 * @param c - the cloud descriptor;
 * @param path - the directory path;
 * @param finfo - a pointer to the file_list structure.
 * @result 0 for success, or error code.
 */
int cld_file_stat(struct cld *c, const char *path, struct file_list *finfo)
{
	int res;
	jsmn_parser p;
	jsmntok_t *tok = NULL;
	size_t tokcount = 0;
	struct memory_struct chunk;

	const char *p_names[] = { "home", "token" };
	const char *p_values[] = { path, c->auth_token };
	char *url = make_url_with_params(c->curl, "file", p_names, p_values, ARRAY_SIZE(p_names));
	
	if (!url)
		return 1;

	memory_struct_init(&chunk);
	res = get_req(c->curl, &chunk, url);
	free(url);
	
	if (res) {
		log_error("Get failed\n");
		return res;
	}

	/* Prepare parser */
	jsmn_init(&p);

	tok = parse_json(&p, chunk.memory, chunk.size, &tokcount);
	if (!tok) {
		printf("Could not parse JSON\n");
		chunk.memory[chunk.size - 1] = 0;
		printf("%s\n", chunk.memory);
		res = 1;
	} else {
		res = parse_file_stat(chunk.memory, tok, finfo);
	}
	
	free(tok);
	memory_struct_cleanup(&chunk);
	return res;
}
