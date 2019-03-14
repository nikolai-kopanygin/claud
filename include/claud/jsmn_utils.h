/**
 * @file jsmn_utils.h
 * Utility functions API for parsing JSON using JSMN library.
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

#ifndef __JSMN_UTILS_H
#define __JSMN_UTILS_H

#include <claud/jsmn.h>

#ifdef __cplusplus
}
#endif

size_t get_json_element_count(jsmntok_t *t);
jsmntok_t *find_json_element_by_name(const char *js, jsmntok_t *t,
		 size_t count, jsmntype_t type, const char *name);
int64_t parse_json_int64(const char *js, jsmntok_t *t);
char *parse_json_string(const char *js, jsmntok_t *t);
jsmntok_t *parse_json(jsmn_parser *p, const char *buf, size_t buf_size,
		      size_t *count);
int64_t get_json_int64_by_name(const char *js, jsmntok_t *t,
			       size_t count, const char *name);
int get_json_int_by_name(const char *js, jsmntok_t *t,
			 size_t count, const char *name);
char *get_json_string_by_name(const char *js, jsmntok_t *t,
			      size_t count, const char *name);
bool get_json_bool_by_name(const char *js, jsmntok_t *t,
			      size_t count, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __JSMN_UTILS_H */
