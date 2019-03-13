/**
 * @file types.h
 * Data types for Mail.Ru Cloud access library.
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

#ifndef __TYPES_H_
#define __TYPES_H_

#include <curl/curl.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AuthToken to unmarshal Auth json data.
 */
struct auth_token {
	struct {
		char *token;
	} body;
};

struct list_item;

/**
 * FileList used to unmarshal json information about files in mail.ru cloud.
 */
struct file_list {
	char *email;
	struct {
		time_t mtime;
		struct {
			int folders;
			int files;
		} count;
		char *tree;
		char *name;
		int grev;
		int64_t size;
		struct {
			char *order;
			char *type;
		} sort;
		char *kind;
		int rev;
		char *type;
		char *home;
		char *hash;
		struct list_item *list;
		size_t nr_list_items;
	} body;
};

/**
 * ListItem used to unmarshal json information about a file.
 */
struct list_item {
	char *name;
	int64_t size;
	char *kind;
	char *type;
	char *home;
	char *hash;
	char *tree;
	time_t mtime;
	char *virus_scan;
	int grev;
	int rev;
	struct {
		int folders;
		int files;
	} count;
};

/**
 * ShardItem holds information for a particular "shard".
 */
struct shard_item {
	char *count;
	char *url;
};

struct shard_item_array {
	size_t size;
	struct shard_item *items;
};

/**
 * ShardInfo used to unmarshal json information about "shards" which contain
 * URL's for API operations.
 */
struct shard_info {
	char *email;
	struct {
		struct shard_item_array upload;
		struct shard_item_array get;
	} body;
	time_t time;
	int status;
};

struct cld *new_cloud(const char *user,
				const char *password,
				const char *domain,
				int *error);
void delete_cloud(struct cld *c);

/**
 * Check whether @finfo describes a directory.
 * @param finfo - the file_list structure.
 * @return true if @finfo describes a directory, otherwise false.
 */
inline static bool file_list_is_dir(const struct file_list *finfo)
{
	return finfo && finfo->body.kind &&
		0 == strcmp(finfo->body.kind, "folder");
}

#ifdef __cplusplus
}
#endif

#endif /* __TYPES_H_ */
