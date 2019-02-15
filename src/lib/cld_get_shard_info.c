#include <malloc.h>
#include <string.h>
#include <curl/curl.h>
#include <claud/types.h>
#include <claud/http_api.h>
#include <claud/cld.h>
#include <claud/jsmn_utils.h>
#include <claud/utils.h>

static void shard_item_cleanup(struct shard_item *item)
{
	free(item->count);
	item->count = NULL;
	free(item->url);
	item->url = NULL;
}

/**
 * @return 0 for success, or 1 for error
 */
static int parse_shard_item(const char *js, jsmntok_t *t, size_t count,
			    struct shard_item *item)
{
	item->count = get_json_string_by_name(js, t, count, "count");
	if (!item->count)
		return 1;

	item->url = get_json_string_by_name(js, t, count, "url");
	if (!item->url)
		return 1;

	return 0;
}

static int parse_shard_item_array_by_name(const char *js, jsmntok_t *tok,
					  size_t count, const char *name,
					  struct shard_item_array *arr)
{
	jsmntok_t *t, *item;
	size_t i;
	
	t = find_json_element_by_name(js, tok, count, JSMN_ARRAY,
						 name);
	if (!t)
		return 1;
	
	arr->items = calloc(t->size, sizeof(struct shard_item));
	if (!arr->items) {
		log_error("Could not alloc shard items");
		return 1;
	}
	arr->size = t->size;
	item = t + 1;
	for (i = 0; i < t->size; i++) {
		size_t cur_count = get_json_element_count(item);
		if (parse_shard_item(js, item, cur_count, &arr->items[i])) {
			log_error("Could not alloc shard items");
			return 1;
		}
		item += cur_count;
	}

	return 0;
}

static int parse_shard_info(const char *js, jsmntok_t *tok, struct shard_info *s)
{
	jsmntok_t *body;
	size_t count, body_count;
	
	count = get_json_element_count(tok);

	if (!(s->email = get_json_string_by_name(js, tok, count, "email"))) {
		log_error("Wrongly formatted shard info\n");
		return 1;
	}
	s->time = (time_t)get_json_int64_by_name(js, tok, count, "time");
	s->status = get_json_int_by_name(js, tok, count, "status");
	body = find_json_element_by_name(js, tok, count, JSMN_OBJECT, "body");
	if (!body) {
		log_error("Wrongly formatted shard info\n");
		return 1;
	}
	body_count = get_json_element_count(body);
	if (parse_shard_item_array_by_name(js, body, body_count,
		                           "upload", &s->body.upload)) {
		log_error("Failed parsing 'upload' shard items\n");
		return 1;
	}
	if (parse_shard_item_array_by_name(js, body, body_count,
		                           "get", &s->body.get)) {
		log_error("Failed parsing 'get' shard items\n");
		return 1;
	}
	return 0;
}

static void shard_info_cleanup(struct shard_info *s)
{
	size_t i;
	free(s->email);
	for (i = 0; i < s->body.upload.size; i++)
		shard_item_cleanup(&s->body.upload.items[i]);
	free(s->body.upload.items);
	for (i = 0; i < s->body.get.size; i++)
		shard_item_cleanup(&s->body.get.items[i]);
	free(s->body.get.items);
}

int cld_get_shard_info(struct cld *c)
{
	int res;
	jsmn_parser p;
	jsmntok_t *tok = NULL;
	size_t tokcount = 0;
	struct memory_struct chunk;

	struct shard_info s;
	const char *p_names[1] = { "token"};
	const char *p_values[1] = { c->auth_token };
	char *url = make_url_with_params(c->curl, "dispatcher", p_names,
					 p_values, 1);
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
		res = parse_shard_info(chunk.memory, tok, &s);
		if (!res) {
			/* Move Get and Upload to c */
			free(c->shard.get);
			c->shard.get = s.body.get.items[0].url;
			s.body.get.items[0].url = NULL;
			free(c->shard.upload);
			c->shard.upload = s.body.upload.items[0].url;
			s.body.upload.items[0].url = NULL;
		}
		free(tok);
	}
	shard_info_cleanup(&s);
	memory_struct_cleanup(&chunk);
	return res;
}
