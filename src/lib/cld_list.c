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
#include <regex.h>
#include <errno.h>

#include <claud/types.h>
#include <claud/http_api.h>
#include <claud/cld.h>
#include <claud/jsmn_utils.h>
#include <claud/utils.h>

static int handle_compounds(struct file_list *contents);

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
			   struct file_list *finfo, bool raw)
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

	if (!raw)
		handle_compounds(finfo);
	
	return 0;
}

/**
 * Read the contents of a mail.ru cloud directory to the specified
 * file_list structure.
 * @param c - the cloud descriptor;
 * @param path - the directory path;
 * @param finfo - a pointer to the file_list structure.
 * @param raw - if true, do not join compound file items.
 * @result 0 for success, or error code.
 */
int cld_get_file_list(struct cld *c, const char *path, struct file_list *finfo,
		      bool raw)
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
		log_error("Could not parse JSON\n");
		chunk.memory[chunk.size - 1] = 0;
		log_error("%s\n", chunk.memory);
		res = 1;
	} else {
		res = parse_file_list(chunk.memory, tok, finfo, raw);
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
	memset(li, 0, sizeof(*li));
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
		log_error("Could not parse JSON\n");
		chunk.memory[chunk.size - 1] = 0;
		log_error("%s\n", chunk.memory);
		res = 1;
	} else {
		res = parse_file_stat(chunk.memory, tok, finfo);
	}
	
	free(tok);
	memory_struct_cleanup(&chunk);
	return res;
}


/**
 * Move all non-empty file_items to the beginning of array of file_items.
 * @param list - pointer to the array;
 * @nr_items - the number of elements in the array.
 * @return the number of non-empty file_items.
 */
static int compress_file_list(struct list_item *list, size_t nr_items)
{
	size_t non_empty_count = 0;
	size_t i, j;
	
	for (i = 0; i < nr_items; i++) {
		if (list[i].name) {
			non_empty_count++;
		} else {
			for (j = i + 1; j < nr_items; j++) {
				if (list[j].name) {
					memcpy(&list[j], &list[i], sizeof(list[i]));
					memset(&list[i], 0, sizeof(list[i]));
				}
			}
		}
	}
	return non_empty_count;
}

static char *get_part_basename(regex_t *re, const char *name)
{
	regmatch_t m[4];
	return (!regexec(re, name, ARRAY_SIZE(m), m, 0)
			&& m[0].rm_so == 0
			&& m[0].rm_eo == strlen(name))
		? xstrndup(name, m[1].rm_eo)
		: NULL;
}

static void add_to_compounds(struct list_item *item, const char *name,
			     struct list_item **compounds, size_t *nr_compounds)
{
	size_t j;

	/* Look for list item in compounds */
	for (j = 0; j < *nr_compounds; j++)
		if (!strcmp(name, compounds[j]->name))
			break;

	if (j == *nr_compounds) { /* Not found - a new compound */
		compounds[j] = item;
		strcpy(compounds[j]->name, name);
		(*nr_compounds)++;
	} else { /* Found - add size and remove from list */
		compounds[j]->size += item->size;
		list_item_cleanup(item);
	}
}

/**
 * Collapse compounds into regular files with greater size.
 *
 * If a file to be uploaded Mail.ru Cloud
 * is bigger than 2GB (which cloud don't support), then it's split to parts 1, 2, 3 etc.
 * all 2GB long.
 *
 * When such file is downloaded again, all these split files are joined back into one.
 *
 * Sample:
 *  name.mkv 3GB long ===> cloud
 *                      => name.mkv.Multifile-Part01 2GB long
 *                      => name.mkv.Multifile-Part02 1GB long
 *              local <===
 *  name.mkv 3GB long <=
 *
 * @param contents - the directory info
 * @retun 0 for success, or error code
 */
static int handle_compounds(struct file_list *contents)
{
	size_t *nr_items_ptr = &contents->body.nr_list_items;
	size_t nr_items = *nr_items_ptr;
	struct list_item *list = contents->body.list;
	regex_t re;
	struct list_item **compounds = NULL; // Array of ptrs to compound list items
	size_t nr_compounds = 0;
	size_t i;

	compounds = xcalloc(nr_items, sizeof(*compounds));
	
	if (regcomp(&re, PART_REGEX, REG_EXTENDED)) {
		log_error("Failed to compile regex\n");
		free(compounds);
		return 1;
	}
	
	for (i = 0; i < nr_items; i++) {
		char *name = get_part_basename(&re, list[i].name);
		if (name) {
			add_to_compounds(&list[i], name, compounds,
					 &nr_compounds);
			free(name);
		}
	}
	
	*nr_items_ptr = compress_file_list(list, nr_items);
	
	free(compounds);
	regfree(&re);
}

/**
 * Check whether name is the name of a partial file for which the
 * basename is specified.
 * @param re - regex pointer;
 * @param basename  -the basename;
 * @param baselen - the basename length;
 * @param name - the name to check.
 * @return number of the part, or -1 if the name is not the part name.
 */
inline static int get_part_number(regex_t *re, const char *basename,
				  size_t baselen, const char *name)
{
	regmatch_t m[4];
	if (!regexec(re, name, ARRAY_SIZE(m), m, 0)
			&& m[0].rm_so == 0
			&& m[0].rm_eo == strlen(name)
			&& m[1].rm_eo - m[1].rm_so == baselen
			&& !strncmp(basename, name + m[1].rm_so, baselen))
		return atoi(name + m[3].rm_so);
	return -1;
}

/**
 * If the file is split into parts, count the number of parts,
 * else return 0 for a single-part file.
 * @param c - the cloud client;
 * @param path - the remote file path.
 * @return the number of parts or a negative error code.
 */
int cld_count_parts(struct cld *c, const char *path)
{
	int res = -ENOENT;
	bool is_mpart = true;
	struct file_list finfo = { 0 };
	char *dirname = copy_dirname(path);
	char *basename = copy_basename(path);
	size_t baselen = strlen(basename);
	size_t nr_items;
	struct list_item *list;
	regex_t re;
	char *parts = NULL;
	int i;
	
	/* Get raw directory contents */
	res = cld_get_file_list(c, dirname, &finfo, true);
	if (res) {
		log_error("No such directory!\n");
		goto out_free_names;
	}
	
	nr_items = finfo.body.nr_list_items;
	list = finfo.body.list;
	parts = xcalloc(nr_items, sizeof(*parts));
	
	if (regcomp(&re, PART_REGEX, REG_EXTENDED)) {
		log_error("Failed to compile regex\n");
		res = -EFAULT;
		goto out_free_parts;
	}

	for (i = 0; i < nr_items; i++) {
		int idx;
		char *name = list[i].name;
		if(!strcmp(list[i].kind, "folder"))
			continue;
		
		/* Check for single part */
		if (!strcmp(basename, name)) {
			is_mpart = false;
			res = 0;
			break;
		}
		
		/* Check for multiple parts */
		idx = get_part_number(&re, basename, baselen, name);
		if (idx >=0 && idx < nr_items) {
			parts[idx] = 1;
			if (idx == 0)
				res = 0;
		}
	}
	
	if (res == 0 && is_mpart)
		while (res < nr_items && parts[res]) res++;

	regfree(&re);
	
out_free_parts:
	free(parts);
	cld_file_list_cleanup(&finfo);
out_free_names:
	free(dirname);
	free(basename);
	return res;
}
