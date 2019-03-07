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

int cld_get_part(struct cld *c, int fd, const char *src)
{
	int res = 0;
	jsmn_parser p;
	jsmntok_t *tok = NULL;
	size_t tokcount = 0;
	struct memory_struct chunk;
	char *url;

	if (cld_get_shard_info(c))
		return 1;
	
	url = malloc(strlen(c->shard.get) + strlen(src) + 1);
	if (!url)
		return 1;
	strcpy(url, c->shard.get);
	strcat(url, src);

	memory_struct_init(&chunk);
	chunk.buf_size = DOWNLOAD_BUFFERSIZE;
	chunk.show_progress = true;
	
	res = get_req(c->curl, &chunk, url);
	free(url);
	
	if (!res) {
		ssize_t written = write(fd, chunk.memory, chunk.size);
		if (written < (ssize_t)chunk.size) {
			log_error("Could not write to file\n");
			res = 1;
		}
	}
	
	memory_struct_cleanup(&chunk);
	return res;
}

/**
 * Download a file specified by its remote path @src
 * to the local path @dst.
 * Multipart files are supported.
 * @param c - the cloud client;
 * @param src - the remote path;
 * @param dst -the local path.
 * @return 0 for success, or error code.
 */
int cld_get(struct cld *c, const char *src, const char *dst)
{
	int res = 0;
	int fd;
	int nr_parts = cld_count_parts(c, src);
	
	if (nr_parts < 0)
		return 1;
	
	if ((fd = creat(dst, 0644)) < 0) {
		log_error("Could not open file to write\n");
		return 1;
	}
	
	if (nr_parts == 0) {
		res = cld_get_part(c, fd, src);
	} else {
		int i;
		for (i = 0; !res && i < nr_parts; i++) {
			char *name = get_file_part_name(src, i);
			res = cld_get_part(c, fd, name);
			free(name);
		}
	}

	if (close(fd)) {
		log_error("Failed to close file\n");
		res = 1;
	}

	
	return res;
}
