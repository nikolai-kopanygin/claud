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
	
	printf("src: dir '%s' base '%s'\n", src_dir, src_base);
	printf("dst: dir '%s' base '%s'\n", dst_dir, dst_base);
	
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
	
	printf("src: dir '%s' base '%s'\n", src_dir, src_base);
	printf("dst: dir '%s' base '%s'\n", dst_dir, dst_base);
	
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

