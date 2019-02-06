#include <libgen.h>
#include <malloc.h>
#include <string.h>
#include <curl/curl.h>
#include "types.h"
#include "http_api.h"
#include "cld.h"
#include "utils.h"

#define COOKIE_FILE "/tmp/cookies.txt"
#define LOGIN_URL "https://auth.mail.ru/cgi-bin/auth"
#define SDC_URL "https://auth.mail.ru/sdc?from=https://cloud.mail.ru/home"

static int login(struct cld *c,
		 const char *user,
		 const char *password,
		 const char *domain)
{
	struct memory_struct chunk;
	memory_struct_init(&chunk);
	
	const char *names[] = { "Domain", "Login", "Password" };
	const char *values[] = { domain, user, password };
	
	int res = post_form_req(c->curl, &chunk, LOGIN_URL, names,
				values, ARRAY_SIZE(names));

	memory_struct_cleanup(&chunk);
	return res;
}

static char *find_token(const char *data, size_t *len) {
	static const char *tag = "\"token\":\"";
	char *start = strstr(data, tag);
	char *end;
	if (!start)
		return NULL;
	start += strlen(tag);
	end = strchr(start, '"');
	if (!end)
		return NULL;
	*len = end - start;
	return start;
}

static char *store_token(const char *data)
{
	char *buf = NULL;
	size_t token_len;
	char *token = find_token(data, &token_len);
	if (!token || !token_len) {
		log_error("Token not found\n");
		return NULL;
	}

	if (!(buf = malloc(token_len + 1))) {
		log_error("Token allocation failed\n");
		return NULL;
	}

	memcpy(buf, token, token_len);
	buf[token_len] = 0;
	
	return buf;
}

/**
 * Request a token over HTTP, allocate a memory
 * buffer and store the token in the buffer.
 * @return a pointer to the buffer for success, or NULL for error
 */
static char *get_token(CURL *curl)
{
	char *buf = NULL;
	int res;
	struct memory_struct chunk;
	char *url = make_url("tokens/csrf");
	if (!url)
		return NULL;

	memory_struct_init(&chunk);
	
	if (!get_req(curl, &chunk, url))
		buf = store_token(chunk.memory);

	memory_struct_cleanup(&chunk);
	free(url);
	
	return buf;
}

// NewCloud authenticates with mail.ru and returns a new object associated with user account.
// domain parameter should be "mail.ru"
struct cld *new_cloud(const char *user,
				const char *password,
				const char *domain,
				int *error)
{
	struct cld *c;
	CURL *curl;
	
	*error = 1; // by default

	if (!(c = calloc(1, sizeof(*c)))) {
		log_error("Could not allocate mail_ru_cloud\n");
		return NULL;
	}

	/* get a curl handle */ 
	if (!(curl = curl_easy_init())) {
		log_error("curl_easy_init() failed");
		free(c);
		return NULL;
	}
	c->curl = curl;

	/* export cookies to this file when closing the handle */
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, COOKIE_FILE);
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, COOKIE_FILE);
	if (login(c, user, password, domain))
		goto cleanup;

	if (get_req(curl, NULL, SDC_URL))
		goto cleanup;
	
	if (!(c->auth_token = get_token(curl)))
		goto cleanup;
	
	/* Success */
	*error = 0;
	return c;
	
	/* Errors */
cleanup:
	curl_easy_cleanup(curl);
	free(c);
	return NULL;
}

void delete_cloud(struct cld *c)
{
	if (get_req(c->curl, NULL, "http://win.mail.ru/cgi-bin/logout"))
		log_error("Logout failed\n");
	
	curl_easy_cleanup(c->curl);
	free(c->shard.get);
	free(c->shard.upload);
	free(c->auth_token);
	free(c);
}
