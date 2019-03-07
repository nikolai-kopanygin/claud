/**
 * @file cld_get.c - remote file download implementation.
 */

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

#define SHARE_BUFFER_SIZE 1024

static char *parse_json_for_share_link(struct memory_struct *chunk)
{
	char *link = NULL;
	char *s;
	size_t tokcount = 0;
	jsmntok_t *tok;
	jsmn_parser p;
	
	jsmn_init(&p);
	if (!(tok = parse_json(&p, chunk->memory, chunk->size, &tokcount))) {
		log_error("Could not parse JSON\n");
		chunk->memory[chunk->size - 1] = 0;
		log_error("%s\n", chunk->memory);
		return NULL;
	}
	
	s = get_json_string_by_name(chunk->memory, tok, tokcount, "body");
	if (s) {
		link = malloc(strlen(PUBLIC_ENDPOINT) + strlen(s) + 1);
		if (link)
			sprintf(link, "%s%s", PUBLIC_ENDPOINT, s);
		free(s);
	}
	free(tok);
	return link;
}

/**
 * Get a string containing the shareable link to a remote file.
 * The string is allocated in memory.
 * @param c - the cloud descriptor;
 * @param path - the file path.
 * @result string pointer for success, or NULL.
 */
char *cld_file_share(struct cld *c, const char *path)
{
	char *link = NULL;
	const char *names[] = { "token", "api", "home" };
	const char *values[] = { c->auth_token, "2", path };
	char *url = make_url("file/publish");
	
	struct memory_struct chunk;
	memory_struct_init(&chunk);
	
	if (post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names))) {
		log_error("file publish failed, Msg: %.*s\n",
			(int)chunk.size, chunk.memory);
	} else {
		link = parse_json_for_share_link(&chunk);
	}

	free(url);
	memory_struct_cleanup(&chunk);
	return link;
}

/**
 * Output buffer context modified by add_part_link.
 */
struct share_ctx {
	char *url; /**< pointer to the buffer */
	int n;	   /**< current buffer size */
	int pos;   /**< current output position to the buffer */
};

static int add_part_link(struct cld *c, const char *path, struct share_ctx *sctx)
{
	char *s = cld_file_share(c, path);
	if (!s) {
		log_error("Could not share link\n");
		return 1;
	}
	while (1) {
		int slen = snprintf(sctx->url + sctx->pos, sctx->n - sctx->pos,
				    "%s: %s\n", path, s);
		if (slen + sctx->pos < sctx->n) {
			sctx->pos += slen;
			break;
		}
		sctx->n = slen + sctx->pos + 1;
		sctx->url = xrealloc(sctx->url, sctx->n);
	}
	free(s);
	return 0;
}

/**
 * Preapare a string containing a shareable URL for
 * the single-part remote file or a list of URL's
 * for the multi-part remote file.
 * Multipart files are supported.
 * @param c - the cloud client;
 * @param path - the remote path;
 * @param url_ptr - the pointer to the resulting string.
 * @return 0 for success, or error code.
 */
int cld_share(struct cld *c, const char *path, char **url_ptr)
{
	int res = 0;
	int fd;
	struct share_ctx sctx = {
		.url = xcalloc(1, SHARE_BUFFER_SIZE),
		.n = SHARE_BUFFER_SIZE - 1,
		.pos = 0
	};
	int nr_parts = cld_count_parts(c, path);
	
	*url_ptr = NULL;
	if (nr_parts < 0) {
		free(sctx.url);
		return 1;
	}
	
	if (nr_parts == 0) {
		res = add_part_link(c, path, &sctx);
		if (res) {
			log_error("Could not share link\n");
			free(sctx.url);
			return 1;
		}
	} else {
		int i = 0;
		for (; res == 0 && i < nr_parts; i++) {
			char *partname = get_file_part_name(path, i);
			res = add_part_link(c, partname, &sctx);
			free(partname);
			if (res) {
				log_error("Could not share link\n");
				free(sctx.url);
				return 1;
			}
		}
	}
	*url_ptr = sctx.url;
	return 0;
}
