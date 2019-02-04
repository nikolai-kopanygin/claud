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

#include "types.h"
#include "command.h"
#include "http_api.h"
#include "mail_ru_cloud.h"
#include "jsmn_utils.h"
#include "utils.h"

static int add_file(struct mail_ru_cloud *c,
		    const char *dst,
		    const char *hash,
		    const char *size)
{
	printf("dst '%s' hash '%s' size '%s'\n", dst, hash, size);
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

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

static int upload_file_part(struct mail_ru_cloud *c, const char *dst, FILE *f,
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
		printf("s0 is NULL\n");
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
	printf("s0: %s\ns1: %s\n", s0, s1);
	res = add_file(c, dst, s0, s1);
	
out_unmap:
	munmap(addr, length);
out:
	memory_struct_cleanup(&chunk);
	return res;
}

int command_upload(struct mail_ru_cloud *c, struct command *cmd)
{
	int res = 0;
	FILE *f;
	struct stat sb;
	const char *src = cmd->args[0];
	const char *dst = cmd->args[1];
	printf("%s\n%s\n", src, dst);
	
	const char *mp_str = ".Multifile-Part";
	char *mpbuf = malloc(strlen(src) + strlen(mp_str) + 10);
	if (!mpbuf) {
		log_error("Out of memory\n");
		return 1;
	}

	if (get_shard_info(c))
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
			//if (part == 0)
			//	continue;
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
