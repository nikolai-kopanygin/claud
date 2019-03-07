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
#include <sys/sysmacros.h>
#include <sys/mman.h>
#include <ctype.h>

#include <claud/types.h>
#include <claud/http_api.h>
#include <claud/cld.h>
#include <claud/jsmn_utils.h>
#include <claud/utils.h>

static int add_file(struct cld *c,
		    const char *dst,
		    const char *hash,
		    const char *size)
{
	log_debug("dst '%s' hash '%s' size '%s'\n", dst, hash, size);
	int res;
	const char *names[] = { "token", "home", "conflict", "hash", "size" };
	const char *values[] = { c->auth_token, dst, "strict", hash, size };
	char *url = make_url("file/add");

	struct memory_struct chunk;
	
	memory_struct_init(&chunk);
	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));
	free(url);
	if (res)
		log_error("add_file failed, Msg: %.*s\n",
			(int)chunk.size, chunk.memory);
	
	memory_struct_cleanup(&chunk);
	return res;
	
}

static int upload_file_part(struct cld *c, const char *dst, FILE *f,
			    size_t length, int part)
{
	int res = 0;
	off_t offset = MAX_FILE_SIZE * part;
	struct memory_struct chunk;
	void *addr;
	struct upload_stream upst = {0,};
	
	memory_struct_init(&chunk);
	chunk.show_progress = true;
	
	addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fileno(f), offset);
	if (!addr) {
		log_error("mmap failed\n");
		res = 1;
		goto out;
	}
	upst.addr = addr;
	upst.left = length;
	
	res = upload_req(c->curl, &chunk, c->shard.upload, dst, &upst);
	if (res) {
		log_error("Could not upload file part\n");
		goto out_unmap;
	}

	chunk.memory[chunk.size-1] = 0;
	char *s = trimwhitespace(chunk.memory);
	char *s0 = strrchr(s, '\n');
	if (!s0) {
		log_debug("s0 is NULL\n");
		s0 = s;
	} else {
		s0 += 1;
	}
	char *s1 = strchr(s0, ';');
	if (s1) {
		*s1++ = 0;
	} else {
		s1 = "";
	}
	log_debug("s0: %s\ns1: %s\n", s0, s1);
	res = add_file(c, dst, s0, s1);
	
out_unmap:
	munmap(addr, length);
out:
	memory_struct_cleanup(&chunk);
	return res;
}

int cld_upload(struct cld *c, const char *src, const char *dst)
{
	int res = 0;
	FILE *f;
	struct stat sb;
	
	static const char *mp_str = PART_SUFFIX;
	char *mpbuf = malloc(strlen(src) + strlen(mp_str) + 10);
	if (!mpbuf) {
		log_error("Out of memory\n");
		return 1;
	}

	if (cld_get_shard_info(c))
		return 1;
	
	if (!(f = fopen(src, "r"))) {
		log_error("Could not open file\n");
		goto out;
	}
	if (fstat(fileno(f), &sb) == -1) {
		log_error("fstat\n");
		res = 1;
		goto out_close;
	}

	if (sb.st_size <= MAX_FILE_SIZE) {
		res = upload_file_part(c, dst, f, sb.st_size, 0);
	} else {
		off_t spos = 0;
		int part = 0;
		for (; spos < sb.st_size && !res; spos += MAX_FILE_SIZE, part++) {
			size_t n = spos + MAX_FILE_SIZE <= sb.st_size
				? MAX_FILE_SIZE
				: sb.st_size % MAX_FILE_SIZE;
			sprintf(mpbuf, "%s%s%02d", dst, mp_str, part);
			res = upload_file_part(c, mpbuf, f, n, part);
		}
	}

out_close:
	if (fclose(f)) {
		log_error("Failed to close file\n");
	}
out:
	free(mpbuf);
	return res;
}
