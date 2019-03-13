/**
 * @file http_api.h
 * HTTP functions API for Mail.Ru Cloud access library.
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

#ifndef __HTTP_API
#define __HTTP_API

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOUD_ENDPOINT "https://cloud.mail.ru/"
#define URL_BASE CLOUD_ENDPOINT "api/v2/"
#define PUBLIC_ENDPOINT CLOUD_ENDPOINT "public/"
#define USER_AGENT "Mozilla / 5.0(Windows; U; Windows NT 5.1; en - US; rv: 1.9.0.1) Gecko / 2008070208 Firefox / 3.0.1"

/* 128K buffer */
#define UPLOAD_BUFFERSIZE (1L << 17)
#define DOWNLOAD_BUFFERSIZE (1L << 17)

/**
 * The mail.ru file size limit for a free account is 2GB.
 * A PAGE of bytes reserved for form data/fields.
 */
#define MAX_FILE_SIZE ((1ULL << 31) - sysconf(_SC_PAGE_SIZE))

#ifdef MARCFS_COMPAT
#define PART_SUFFIX ".marcfs-part-"
#define PART_FORMAT "%s%s%d"
#else
#define PART_SUFFIX ".Multifile-Part"
#define PART_FORMAT "%s%s%02d"
#endif
#define PART_REGEX "(.+)(\\" PART_SUFFIX ")([[:digit:]]+)"

struct CURL;

/**
 * A memory structure receiving the HTTP response.
 */
struct memory_struct {
	char *memory;		/**< The memory buffer pointer */
	size_t size;		/**< The memory buffer size */
	size_t buf_size; 	/**< Upload or download buffer size */
	bool show_progress;	/**< Whether to show progress while uploading or downloading files */
};

/**
 * A file upload stream descriptor
 */
struct upload_stream {
	FILE *f;	/**< The file descriptor used with file I/O API */
	void *addr;	/**< The memory pointer used when data is uploaded from memory */
	size_t left;	/**< The amount of data left to be uploaded */
};

/**
 * Type of progress to display
 */
enum progress_type {
  PROGRESS_NONE = 0,  /* NONE must stay at 0 */
  PROGRESS_STARTED,
  PROGRESS_DONE
};

/**
 * To initialize zero out the struct then set session
 */
struct progress_data {
  CURL *session;             /**< curl easy_handle to the calling session */
  int percent;               /**< last percent output to stderr */
  time_t time;               /**< last time progress was output to stderr */
  enum progress_type type;   /**< progress type */
};

void memory_struct_init(struct memory_struct *mem);
void memory_struct_cleanup(struct memory_struct *mem);
void memory_struct_reset(struct memory_struct *mem);

int http_req(CURL *curl, struct memory_struct *chunk, const char *url);
int get_req(CURL *curl, struct memory_struct *chunk, const char *url);
int post_form_req(CURL *curl,
	     struct memory_struct *chunk,
	     const char *url,
	     const char *names[],
	     const char *values[],
	     size_t nr_params);
int post_req(CURL *curl,
	     struct memory_struct *chunk,
	     const char *url,
	     const char *names[],
	     const char *values[],
	     size_t nr_params);

int upload_req(CURL *curl,
	       struct memory_struct *chunk,
	       const char *url,
	       const char *dst,
	       struct upload_stream *upst);

char *make_url(const char *s);
char *make_url_with_params(struct CURL *curl,
			   const char *s,
			   const char *names[],
			   const char *values[],
			   size_t nr_params);

#ifdef __cplusplus
}
#endif

#endif /* __HTTP_API */
