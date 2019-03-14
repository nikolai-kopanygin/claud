/**
 * @file cld_commands.c
 * Implementation of API for Mail.Ru Cloud access library.
 *
 * Copyright (C) 2019 Nikolai Kopanygin <nikolai.kopanygin@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <curl/curl.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <libgen.h>
#include <claud/types.h>
#include <claud/http_api.h>
#include <claud/cld.h>
#include <claud/jsmn_utils.h>
#include <claud/utils.h>

/**
 * Delete a remote file or directory specified by path.
 * @param c - the cloud descriptor;
 * @param path - the file or directory path.
 * @return 0 for success, or error code.
 */
int cld_remove(struct cld *c, const char *path)
{
	int res;
	const char *names[] = { "token", "api", "home" };
	const char *values[] = { c->auth_token, "2", path };
	char *url = make_url("file/remove");
	
	struct memory_struct chunk;
	memory_struct_init(&chunk);

	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));
	if (res)
		log_error("command_remove failed, Msg: %.*s\n",
			(int)chunk.size, chunk.memory);
	
	free(url);
	memory_struct_cleanup(&chunk);

	return res;
}

/**
 * Create a remote directory specified by path.
 * @param c - the cloud descriptor;
 * @param path - the directory path.
 * @return 0 for success, or error code.
 */
int cld_mkdir(struct cld *c, const char *path)
{
	int res;
	const char *names[] = { "token", "api", "home", "conflict" };
	const char *values[] = { c->auth_token, "2", path, "strict" };
	char *url = make_url("folder/add");
	
	struct memory_struct chunk;
	memory_struct_init(&chunk);

	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));
	if (res)
		log_error("c_mkdir failed, Msg: %.*s\n",
			(int)chunk.size, chunk.memory);
	
	free(url);
	memory_struct_cleanup(&chunk);

	return res;
}

/**
 * Move a remote file to the specified directory.
 * @param c - the cloud descriptor;
 * @param src - the original file path;
 * @param target_dir - the destination directory path.
 * @return 0 for success, or error code.
 */
static int c_move_a(struct cld *c, const char *src,
		  const char *target_dir)
{
	int res;
	const char *names[] =
		{ "token", "api", "conflict", "home", "folder" };
	const char *values[] =
		{ c->auth_token, "2", "strict", src, target_dir };
	char *url = make_url("file/move");

	struct memory_struct chunk;
	
	memory_struct_init(&chunk);

	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));
	free(url);

	memory_struct_cleanup(&chunk);
	return res;
}

/**
 * Rename a remote file.
 * @param c - the cloud descriptor;
 * @param src - the original file path;
 * @param target_name - the new file name.
 * @return 0 for success, or error code.
 */
static int c_rename(struct cld *c, const char *src,
		  const char *target_name)
{
	int res;
	const char *names[] =
		{ "token", "api", "conflict", "home", "name" };
	const char *values[] =
		{ c->auth_token, "2", "strict", src, target_name };
	char *url = make_url("file/rename");

	struct memory_struct chunk;
	memory_struct_init(&chunk);

	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));

	free(url);
	memory_struct_cleanup(&chunk);

	return res;
}

/**
 * Check whether the specified path is a remote directory.
 * @param c - the cloud descriptor;
 * @param path - the file or directory path.
 * @return true if the path is a directory, false otherwise.
 */
static bool is_dir(struct cld *c, const char *path) {
	bool res = false;
	struct file_list finfo = { 0 };
	
	res = !cld_file_stat(c, path, &finfo) && file_list_is_dir(&finfo);
	
	cld_file_list_cleanup(&finfo);
	return res;
}

/**
 * Rename a remote source file or directory to the specified
 * destination path.
 * If the destination path is an exiting directory, the source
 * file or directory is moved to that directory.
 * @param c - the cloud descriptor;
 * @param src - the source file or directory path;
 * @param dst - the destination file or directory path.
 * @return 0 for success, or error code.
 */
int cld_move(struct cld *c, const char *src, const char *dst)
{
	int res = 0;
	char tmp_dir[12] = "/tmp";
	char *src_dir, *src_base, *dst_dir, *dst_base;
	char *src_tmp = NULL;
	char *dst_tmp = NULL;
	
	src_dir = copy_dirname(src);
	src_base = copy_basename(src);

	if (is_dir(c, dst)) {
		dst_dir = strdup(dst);
		dst_base = strdup(src_base);
	} else {
		dst_dir = copy_dirname(dst);
		dst_base = copy_basename(dst);
	}
	
	/* Make up tmp_dir name */
	fill_random(tmp_dir + strlen(tmp_dir), ARRAY_SIZE(tmp_dir) - strlen(tmp_dir));
	
	if (!src_dir || !src_base || !dst_dir || !dst_base) {
		log_error("Could not allocate memory\n");
		res = 1;
		goto cleanup;
	}

	/* If two directories are the same, just rename */
	if (!strcmp(src_dir, dst_dir)) {
		res = c_rename(c, src, dst_base);
		goto cleanup;
	}
	/* If two names are the same, just move */
	if (!strcmp(src_base, dst_base)) {
		res = c_move_a(c, src, dst_dir);
		goto cleanup;
	}
	/*
	 * At first it seemed we could rename the file, then move it, or
	 * vice versa. But there may be the case when there already is
	 * a at file src_dir/dst_base or dst_dir/src_base.
	 * So, to do it safely, we create a temporary directory,
	 * move our file to there, rename it there, and then move the file
	 * to the destination directory.
	 */
	
	if (cld_mkdir(c, tmp_dir)) {
		res = 1;
		goto cleanup;
	}
	if (c_move_a(c, src, tmp_dir)) {
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
		/* roll back moving */
		c_move_a(c, src_tmp, src_dir);
		res = 1;
		goto rm_tmp_dir;
	}
	if (c_move_a(c, dst_tmp, dst_dir)) {
		/* roll back renaming and moving */
		c_rename(c, dst_tmp, src_base);
		c_move_a(c, src_tmp, src_dir);
		res = 1;
		goto rm_tmp_dir;
	}
	
rm_tmp_dir:
	if (cld_remove(c, tmp_dir))
		res = 1;
cleanup:
	free(dst_tmp);
	free(src_tmp);
	free(dst_dir);
	free(src_dir);
	free(dst_base);
	free(src_base);
	return res;
}

/**
 * Copy a remote file or a directory to another directory,
 * keeping the original file or directory base name.
 * @param c - the cloud descriptor;
 * @param src - the original file or directory path;
 * @param target_dir - the destination directory.
 * @return 0 for success, or error code.
 */
static int c_copy_a(struct cld *c, const char *src,
		  const char *target_dir)
{
	int res;
	const char *names[] =
		{ "token", "api", "conflict", "home", "folder" };
	const char *values[] =
		{ c->auth_token, "2", "strict", src, target_dir };
	char *url = make_url("file/copy");
	
	struct memory_struct chunk;
	memory_struct_init(&chunk);

	res = post_req(c->curl, &chunk, url, names, values, ARRAY_SIZE(names));

	free(url);
	memory_struct_cleanup(&chunk);

	return res;
}

/**
 * Copy a remote source file or directory to the specified
 * destination path.
 * If the destination path is an exiting directory, the source
 * file or directory is copied to that directory.
 * @param c - the cloud descriptor;
 * @param src - the source file or directory path;
 * @param dst - the destination file or directory path.
 * @return 0 for success, or error code.
 */
int cld_copy(struct cld *c, const char *src, const char *dst)
{
	int res = 0;
	char tmp_dir[12] = "/tmp";
	char *src_dir, *src_base, *dst_dir, *dst_base;
	char *src_tmp = NULL;
	char *dst_tmp = NULL;
	
	src_dir = copy_dirname(src);
	src_base = copy_basename(src);

	if (is_dir(c, dst)) {
		dst_dir = strdup(dst);
		dst_base = strdup(src_base);
	} else {
		dst_dir = copy_dirname(dst);
		dst_base = copy_basename(dst);
	}
	
	/* Make up tmp_dir name */
	fill_random(tmp_dir + strlen(tmp_dir), ARRAY_SIZE(tmp_dir) - strlen(tmp_dir));

	if (!src_dir || !src_base || !dst_dir || !dst_base) {
		log_error("Could not allocate memory\n");
		res = 1;
		goto cleanup;
	}

	if (cld_mkdir(c, tmp_dir)) {
		res = 1;
		goto cleanup;
	}
	if (c_copy_a(c, src, tmp_dir)) {
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
	if (c_copy_a(c, dst_tmp, dst_dir)) {
		res = 1;
		goto rm_tmp_dir;
	}
	
rm_tmp_dir:
	if (cld_remove(c, tmp_dir))
		res = 1;
cleanup:
	free(dst_tmp);
	free(src_tmp);
	free(dst_dir);
	free(src_dir);
	free(dst_base);
	free(src_base);
	return res;
}

static int parse_space_info(const char *js, jsmntok_t *tok, struct space_info *info)
{
	jsmntok_t *body;
	size_t count, body_count;
	
	count = get_json_element_count(tok);
/*
	if (!(info->email = get_json_string_by_name(js, tok, count, "email"))) {
		log_error("Wrongly formatted space info\n");
		return 1;
	}
*/	
	info->time = (time_t)get_json_int64_by_name(js, tok, count, "time");
	info->status = get_json_int_by_name(js, tok, count, "status");
	body = find_json_element_by_name(js, tok, count, JSMN_OBJECT, "body");
	if (!body) {
		log_error("Wrongly formatted space info\n");
		return 1;
	}
	body_count = get_json_element_count(body);
	info->body.bytes_total = (uint64_t)get_json_int64_by_name(js, body, body_count, "bytes_total");
	info->body.bytes_used = (uint64_t)get_json_int64_by_name(js, body, body_count, "bytes_used");
	info->body.overquota = get_json_bool_by_name(js, body, body_count, "overquota");
	return 0;
}

/**
 * Get space info.
 * @param c - the cloud descriptor;
 * @param info - pointer to the structure accepting the info.
 * @return 0 for success, or error code.
 */
int cld_df(struct cld *c, struct space_info *info)
{

	int res;
	jsmn_parser p;
	jsmntok_t *tok = NULL;
	size_t tokcount = 0;
	struct memory_struct chunk;

	const char *names[] = { "token", "api" };
	const char *values[] = { c->auth_token, "2" };
	char *url = make_url_with_params(c->curl, "user/space", names,
					 values, ARRAY_SIZE(names));
	if (!url)
		return 1;
	
	memory_struct_init(&chunk);
	res = get_req(c->curl, &chunk, url);
	free(url);

	if (res) {
		log_error("Get failed\n");
		memory_struct_cleanup(&chunk);
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
		res = parse_space_info(chunk.memory, tok, info);
		free(tok);
	}
	
	memory_struct_cleanup(&chunk);
	return res;
}
