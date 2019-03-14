/**
 * @file http_api.c
 * Implementation of HTTP API for Mail.Ru Cloud access library.
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

#include <libgen.h>
#include <malloc.h>
#include <string.h>
#include <curl/curl.h>
#include <claud/types.h>
#include <claud/utils.h>
#include <claud/http_api.h>

void memory_struct_init(struct memory_struct *mem) {
	mem->memory = malloc(1);
	mem->buf_size = 0;
	mem->size = 0;
	mem->show_progress = false;
}

void memory_struct_cleanup(struct memory_struct *mem) {
	free(mem->memory);
}

void memory_struct_reset(struct memory_struct *mem) {
	memory_struct_cleanup(mem);
	memory_struct_init(mem);
}

static void print_cookies(CURL *curl)
{
	CURLcode res;
	struct curl_slist *cookies;
	struct curl_slist *nc;
	int i;

	log_debug("Cookies, curl knows:\n");
	res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
	if (res != CURLE_OK)
	{
		log_error("Curl curl_easy_getinfo failed: %s\n",
			curl_easy_strerror(res));
		return;
	}
	nc= cookies, i = 1;
	while (nc)
	{
		log_debug("[%d]: %s\n", i, nc->data);
		nc = nc->next;
		i++;
	}
	if (i == 1)
		log_debug("(none)\n");

	curl_slist_free_all(cookies);
}

static int progress_callback(void *clientp, curl_off_t dltotal,
	curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	int percent;
	time_t timenow;
	struct progress_data *d = (struct progress_data *)clientp;

	(void)dltotal;
	(void)dlnow;

#if 0
	log_error("\ndltotal: " CURL_FORMAT_OFF_T
		", dlnow: " CURL_FORMAT_OFF_T
		", ultotal: " CURL_FORMAT_OFF_T
		", ulnow: " CURL_FORMAT_OFF_T
		", time: " CURL_FORMAT_OFF_T "\n",
		dltotal, dlnow, ultotal, ulnow, (curl_off_t)time(NULL));
#endif

	if (d->type == PROGRESS_DONE || ultotal <= 0 || ulnow < 0)
		return 0;
	else if (!ulnow)
		percent = 0;
	else if (ulnow >= ultotal)
		percent = 100;
	else if(ultotal < 10000)
		percent = (int)(ulnow * 100 / ultotal);
	else
		percent = (int)(ulnow / (ultotal / 100));

	if (d->type == PROGRESS_STARTED && d->percent == percent)
		return 0;

	timenow = time(NULL);

	if (timenow == d->time && percent != 100)
		return 0;

	log_error("\r%3d%% uploaded...", percent);

	d->percent = percent;
	d->time = timenow;
	if (percent == 100) {
		log_error("\n\n");
		d->type = PROGRESS_DONE;
	} else {
		d->type = PROGRESS_STARTED;
	}

	return 0;
}

static size_t
write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct memory_struct *mem = (struct memory_struct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if(ptr == NULL) {
		/* out of memory! */ 
		log_error("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

char *make_url(const char *route)
{
	char *buf = malloc(strlen(URL_BASE) + strlen(route) + 1);
	if (!buf)
		log_error("Allocation failed\n");
	else
		sprintf(buf, "%s%s", URL_BASE, route);
	return buf;
}

/**
 * Allocate a string and fill it with an URL-encoded parameter list.
 * @param curl - CURL handle used for encoding;
 * @param names - array of parameter names;
 * @param values - array of parameter values;
 * @param nr_params - the number of parameters.
 * @return pointer to the allocated string for success, or NULL for error.
 */
char *make_params(CURL *curl, const char *names[], const char *values[],
		  size_t nr_params)
{
	char *buf = NULL;
	size_t buf_len = 0;
	int i;
	/* URL-escaped values */
	char **esc_values = calloc(nr_params * 2, sizeof(char *));
	if (!esc_values)
		return NULL;
	for (i = 0; i < nr_params; i++) {
		if (!names[i] || !values[i]) {
			log_error("Empty HTTP request parameter\n");
			goto free_esc_values;
		}
	
		esc_values[i * 2] = curl_easy_escape(curl, names[i], 0);
		if (!esc_values[i * 2])
			goto free_esc_values;
		esc_values[i * 2 + 1] = curl_easy_escape(curl, values[i], 0);
		if (!esc_values[i * 2 + 1])
			goto free_esc_values;
		buf_len += strlen(esc_values[i * 2]) + 
			   strlen(esc_values[i * 2 + 1]) + 2;
 	}
 	/* For empty parameter list, we return an empty string */
 	if (buf_len == 0)
		buf_len = 1;

	if (!(buf = malloc(buf_len))) {
		log_error("Allocation failed\n");
		goto free_esc_values;
	}

	buf[0] = '\0';
	for (i = 0; i < nr_params; i++) {
		sprintf(buf + strlen(buf), "%s%s=%s", (i == 0 ? "" : "&"),
			esc_values[i * 2], esc_values[i * 2 + 1]);
	}
	
free_esc_values:	
	for (i = 0; i < nr_params * 2; i++) {
		curl_free(esc_values[i]);
	}
	free(esc_values);
	return buf;
}

char *make_url_with_params(struct CURL *curl,
			   const char *route,
			   const char *names[],
			   const char *values[],
			   size_t nr_params)
{
	char *buf = NULL;
	char *param_str;
	size_t buf_len;

	if (nr_params == 0)
		return make_url(route);
	
	if (!(param_str = make_params(curl, names, values, nr_params)))
		return NULL;

	buf_len = strlen(URL_BASE) + strlen(route) + strlen(param_str) + 2;
	if (!(buf = malloc(buf_len)))
		log_error("Allocation failed\n");
	else
		sprintf(buf, "%s%s?%s", URL_BASE, route, param_str);
	
	free(param_str);
	return buf;
}

static void fill_post_form(curl_mime *mime,
			   const char *names[],
			   const char *values[],
			   size_t nr_params)
{
	size_t i;
	curl_mimepart *part;
	for (i = 0; i < nr_params; i++) {
		part = curl_mime_addpart(mime);
		curl_mime_name(part, names[i]);
		curl_mime_data(part, values[i], CURL_ZERO_TERMINATED);
		part = curl_mime_addpart(mime);
	}
}

int http_req(CURL *curl, struct memory_struct *chunk, const char *url)
{
	CURLcode res;
	int resp_code;
	struct curl_slist *cookies = NULL;
	struct progress_data progress_data = { 0, };

	log_debug("URL: %s\n", url); 

	curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	if (chunk) {
		if (chunk->show_progress) {
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
					 progress_callback);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA,
					 &progress_data);
			progress_data.session = curl;
		} else {
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		}

		/* Set buffer size to receive data */
		if (chunk->buf_size)
			curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, chunk->buf_size);
		/* send all data to this function  */ 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
		/* we pass our 'chunk' struct to the callback function */ 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
	} else {
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	}

	res = curl_easy_perform(curl);

	/* Parse result */
	res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
	if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code) !=
			CURLE_OK) {
		log_error("curl_easy_getinfo failed: %s\n",
					curl_easy_strerror(res));
		resp_code = 0;
	}
	
	/* Check for errors */ 
	if (res != CURLE_OK) {
		log_error("HTTP request failed: %s\n", curl_easy_strerror(res));
		return 1;
	}

	if (resp_code != 200 && resp_code != 302) {
		log_error("HTTP response code: %d\n", resp_code);
		return 1;
	}

	return 0;
}

int post_req(CURL *curl,
	     struct memory_struct *chunk,
	     const char *url,
	     const char *names[],
	     const char *values[],
	     size_t nr_params)
{
	int res;
	char *param_str;
	
	if (!(param_str= make_params(curl, names, values, nr_params)))
		return 1;
	
	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, param_str);	
	
	res = http_req(curl, chunk, url);
	free(param_str);

	return res;
}

int post_form_req(CURL *curl,
	     struct memory_struct *chunk,
	     const char *url,
	     const char *names[],
	     const char *values[],
	     size_t nr_params)
{
	int res;
	curl_mime *mime;

	curl_easy_reset(curl);
	
	if (nr_params > 0) {
		mime = curl_mime_init(curl);
		fill_post_form(mime, names, values, nr_params);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
	}
	
	res = http_req(curl, chunk, url);

	if (nr_params > 0)
		curl_mime_free(mime);
	
	return res;
}

int get_req(CURL *curl, struct memory_struct *chunk, const char *url)
{
	curl_easy_reset(curl);
	return http_req(curl, chunk, url);
}

static size_t read_callback_mm(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct upload_stream *upst = (struct upload_stream *)stream;
	size_t copy_size = nmemb * size;
	if (copy_size > upst->left)
		copy_size = upst->left;

	memcpy(ptr, upst->addr, copy_size);
	upst->left -= copy_size;
	upst->addr += copy_size;

	return copy_size / size;
}

static size_t read_callback_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t retcode = 0;
	struct upload_stream *upst = (struct upload_stream *)stream;
	size_t copy_memb = nmemb;
	size_t left_memb = upst->left / size;
	if (copy_memb > left_memb)
		copy_memb = left_memb;

	retcode = fread(ptr, size, copy_memb, upst->f);
	if (retcode > 0)
		upst->left -= retcode * size;

	return retcode;
}

static inline size_t
read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct upload_stream *upst = (struct upload_stream *)stream;
	
	if (upst->left == 0) {
		log_error("Sending complete\n");
		return 0;
	}
	
	log_error("*** %zu bytes left to send\n", upst->left);
	return upst->f
		? read_callback_file(ptr, size, nmemb, stream)
		: read_callback_mm(ptr, size, nmemb, stream);
}

// TODO: Move to https://curl.haxx.se/libcurl/c/curl_mime_data_cb.html
int upload_req(CURL *curl,
	       struct memory_struct *chunk,
	       const char *url,
	       const char *dst,
	       struct upload_stream *upst)
{
	int res;
	struct curl_slist *cookies = NULL;
	struct curl_slist *header_list = NULL;
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;

	char *filename = copy_basename(dst);
	if (!filename) {
		log_error("Failed to allocate filename\n");
		return 1;
	}
	
	curl_easy_reset(curl);
	
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "file",
		CURLFORM_FILENAME, filename,
		CURLFORM_STREAM, upst,
		CURLFORM_CONTENTLEN, upst->left,
		CURLFORM_END);
	
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

#ifdef CURLOPT_UPLOAD_BUFFERSIZE
	/* Supported since libcurl v.7.62 */
	curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, UPLOAD_BUFFERSIZE);
#endif
	
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback_mm);
	curl_easy_setopt(curl, CURLOPT_READDATA, (void *)upst);

	/* Include server headers in the output */
	curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
	header_list = curl_slist_append(header_list, "Expect:");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
	
	
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);

	res = http_req(curl, chunk, url);
	curl_slist_free_all(header_list);
	curl_formfree(formpost);
	free(filename);
	
	return res;
}
