/**
 * @file cld.h
 * Exported functions API for Mail.Ru Cloud access library.
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

#ifndef __MAIL_RU_CLOUD_H
#define __MAIL_RU_CLOUD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <claud/types.h>

struct CURL;
struct interface;
struct file_list;

/**
 * MailRuCloud holds all information which required for the api operations.
 */
struct cld {
	struct CURL *curl;
	char *auth_token;
	struct {
		char *get;
		char *upload;
	} shard;
	void (*io_progress)(int64_t, struct interface *interface);
	struct interface *(*init_io_progress)(int64_t);
};

struct cld *new_cloud(const char *user,
				const char *password,
				const char *domain,
				int *error);
void delete_cloud(struct cld *c);

int cld_get_shard_info(struct cld *c);

int cld_mkdir(struct cld *c, const char *path);
int cld_remove(struct cld *c, const char *path);
int cld_move(struct cld *c, const char *src, const char *dst);
int cld_copy(struct cld *c, const char *src, const char *dst);
int cld_get(struct cld *c, const char *src, const char *dst);
int cld_get_part(struct cld *c, int fd, const char *src);
int cld_upload(struct cld *c, const char *src, const char *dst);
int cld_file_stat(struct cld *c, const char *path, struct file_list *finfo);
void cld_file_list_cleanup(struct file_list *finfo);
int cld_get_file_list(struct cld *c, const char *path, struct file_list *finfo,
		      bool raw);
int cld_share(struct cld *c, const char *path, char **link);
int cld_count_parts(struct cld *c, const char *path);
int cld_df(struct cld *c, struct space_info *info);
int cld_create(struct cld *c, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* __MAIL_RU_CLOUD_H */
