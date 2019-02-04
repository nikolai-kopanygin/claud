#include <curl/curl.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <libgen.h>
#include "types.h"
#include "command.h"
#include "http_api.h"
#include "mail_ru_cloud.h"
#include "jsmn_utils.h"
#include "utils.h"

int c_remove(struct mail_ru_cloud *c, const char *path)
{
	int res;
	const char *names[] = { "token", "api", "home" };
	const char *values[] = { c->auth_token, "2", path };
	char *url = make_url("file/remove");
	
	struct memory_struct chunk;
	
	memory_struct_init(&chunk);
	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));
	free(url);
	if (res)
		log_error("command_remove failed, Msg: %.*s\n",
			(int)chunk.size, chunk.memory);
	
	memory_struct_cleanup(&chunk);
	return res;
}

int command_remove(struct mail_ru_cloud *c, struct command *cmd)
{
	return c_remove(c, cmd->args[0]);
}

static int c_mkdir(struct mail_ru_cloud *c, const char *path)
{
	int res;
	const char *names[] = { "token", "api", "home", "conflict" };
	const char *values[] = { c->auth_token, "2", path, "strict" };
	char *url = make_url("folder/add");
	
	struct memory_struct chunk;
	
	memory_struct_init(&chunk);
	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));
	free(url);
	if (res)
		log_error("c_mkdir failed, Msg: %.*s\n",
			(int)chunk.size, chunk.memory);
	
	memory_struct_cleanup(&chunk);
	return res;
}

int command_mkdir(struct mail_ru_cloud *c, struct command *cmd)
{
	return c_mkdir(c, cmd->args[0]);
}

static int c_move_a(struct mail_ru_cloud *c, const char *src,
		  const char *target_dir)
{
	int res;
	const char *names[] =
		{ "token", "api", "conflict", "home", "folder" };
	const char *values[] =
		{ c->auth_token, "2", "strict", src, target_dir };
	char *url = make_url("file/move");
	
	res = post_req(c->curl, NULL, url, names, values, ARRAY_SIZE(names));
	free(url);

	return res;
}

static int c_rename(struct mail_ru_cloud *c, const char *src,
		  const char *target_name)
{
	int res;
	const char *names[] =
		{ "token", "api", "conflict", "home", "name" };
	const char *values[] =
		{ c->auth_token, "2", "strict", src, target_name };
	char *url = make_url("file/rename");
	
	res = post_req(c->curl, NULL, url, names, values, ARRAY_SIZE(names));
	free(url);

	return res;
}

//TODO: This must be reworked to distinguish between files and
// folders in dst path.
int command_move(struct mail_ru_cloud *c, struct command *cmd)
{
	int res = 0;
	static const char *tmp_dir = "/__tmrc";
	char *src = strdup(cmd->args[0]);
	if (!src) {
		log_error("Could not allocate memory\n");
		return 1;
	}
	char *dst = strdup(cmd->args[1]);
	if (!dst) {
		log_error("Could not allocate memory\n");
		free(src);
		return 1;
	}
	char *src_dir = dirname(src);
	char *src_base = basename(src);
	char *dst_dir = dirname(dst);
	char *dst_base = basename(dst);
	char *src_tmp = NULL;
	char *dst_tmp = NULL;

	if (!strcmp(src_dir, dst_dir)) {
		res = c_rename(c, cmd->args[0], dst_base);
		goto cleanup;
	}
	if (!strcmp(src_base, dst_base)) {
		res = c_move_a(c, cmd->args[0], dst_dir);
		goto cleanup;
	}
	if (c_mkdir(c, tmp_dir)) {
		res = 1;
		goto cleanup;
	}
	if (c_move_a(c, cmd->args[0], tmp_dir)) {
		res = 1;
		goto rm_tmp_dir;
	}
	src_tmp = malloc(strlen(tmp_dir) + strlen(src_base) + 2);
	dst_tmp = malloc(strlen(tmp_dir) + strlen(dst_base) + 2);
	if (!src_tmp || !dst_tmp) {
		res = 1;
		goto rm_tmp_dir;
	}
	sprintf(src_tmp, "%s/%s", tmp_dir, src_base);
	sprintf(dst_tmp, "%s/%s", tmp_dir, dst_base);
	if (c_rename(c, src_tmp, dst_base)) {
		res = 1;
		goto rm_tmp_dir;
	}
	if (c_move_a(c, dst_tmp, dst_dir)) {
		res = 1;
		goto rm_tmp_dir;
	}
	
rm_tmp_dir:
	if (c_remove(c, tmp_dir))
		res = 1;
cleanup:
	free(dst_tmp);
	free(src_tmp);
	free(dst);
	free(src);
	return res;
}

// CopyA method is the direct call to api url.
// It does not support rename. src is full source file path,
// targetDir is the directory to copy file to.
static int c_copy_a(struct mail_ru_cloud *c, const char *src,
		  const char *target_dir)
{
	int res;
	const char *names[] =
		{ "token", "api", "conflict", "home", "folder" };
	const char *values[] =
		{ c->auth_token, "2", "strict", src, target_dir };
	char *url = make_url("file/copy");
	
	res = post_req(c->curl, NULL, url, names, values, ARRAY_SIZE(names));
	free(url);

	return res;
}

//TODO: This must be reworked to distinguish between files and
// folders in dst path.
// Copy is convenient method to move files at the mail.ru cloud.
// src and dst should be the full source and destination file paths.
int command_copy(struct mail_ru_cloud *c, struct command *cmd)
{
	int res = 0;
	static const char *tmp_dir = "/__tmrc";
	char *src = strdup(cmd->args[0]);
	if (!src) {
		log_error("Could not allocate memory\n");
		return 1;
	}
	char *dst = strdup(cmd->args[1]);
	if (!dst) {
		log_error("Could not allocate memory\n");
		free(src);
		return 1;
	}
	char *src_dir = dirname(src);
	char *src_base = basename(src);
	char *dst_dir = dirname(dst);
	char *dst_base = basename(dst);
	char *src_tmp = NULL;
	char *dst_tmp = NULL;
	
	printf("src: dir '%s' base '%s'\n", src_dir, src_base);
	printf("dst: dir '%s' base '%s'\n", dst_dir, dst_base);

	if (c_mkdir(c, tmp_dir)) {
		res = 1;
		goto cleanup;
	}
	if (c_copy_a(c, cmd->args[0], tmp_dir)) {
		res = 1;
		goto rm_tmp_dir;
	}
	src_tmp = malloc(strlen(tmp_dir) + strlen(src_base) + 2);
	dst_tmp = malloc(strlen(tmp_dir) + strlen(dst_base) + 2);
	if (!src_tmp || !dst_tmp) {
		res = 1;
		goto rm_tmp_dir;
	}
	sprintf(src_tmp, "%s/%s", tmp_dir, src_base);
	sprintf(dst_tmp, "%s/%s", tmp_dir, dst_base);
	printf("src_tmp: '%s'\n dst_tmp '%s'\n", src_tmp, dst_tmp);
	if (c_rename(c, src_tmp, dst_base)) {
		res = 1;
		goto rm_tmp_dir;
	}
	if (c_copy_a(c, dst_tmp, dst_dir)) {
		res = 1;
		goto rm_tmp_dir;
	}
	
rm_tmp_dir:
	if (c_remove(c, tmp_dir))
		res = 1;
cleanup:
	free(dst_tmp);
	free(src_tmp);
	free(dst);
	free(src);
	return res;
}

