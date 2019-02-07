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

int cld_get(struct cld *c, const char *src, const char *dst)
{
	int res;
	int fd;
	printf("%s\n%s\n", src, dst);
	const char *mp_str = ".Multifile-Part";
	bool mpart = false;
	struct file_list finfo;
	memset(&finfo, 0, sizeof(finfo));
	
	char *mpbuf = malloc(strlen(src) + strlen(mp_str) + 10);
	if (!mpbuf) {
		log_error("Out of memory\n");
		return 1;
	}

	if ((res = cld_file_stat(c, src, &finfo))) {
		cld_file_list_cleanup(&finfo);
		sprintf(mpbuf, "%s%s%02d", src,mp_str, 0);
		res = cld_file_stat(c, mpbuf, &finfo);
		mpart = true;
	}
	if (res)
		goto out;

	if ((fd = creat(dst, 0644)) < 0) {
		log_error("Could not open file to write\n");
		goto out;
	}
	
	if (mpart) {
		int i;
		for (i = 0;; i++) {
			cld_file_list_cleanup(&finfo);
			sprintf(mpbuf, "%s%s%02d", src,mp_str, i);
			if (!(res = cld_file_stat(c, mpbuf, &finfo))) {
				if ((res = cld_get_part(c, fd, mpbuf))) {
					log_error("Could not get file\n");
					break;
				}
				
			} else {
				if (i > 0)
					res = 0;
				break;
			}
			
		}
	} else {
		res = cld_get_part(c, fd, src);
	}

out_close:
	if (close(fd)) {
		log_error("Failed to close file\n");
	}
out:
	cld_file_list_cleanup(&finfo);	
	free(mpbuf);
	return res;
}
