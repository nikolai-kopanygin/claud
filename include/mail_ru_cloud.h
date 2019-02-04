#ifndef __MAIL_RU_CLOUD_H
#define __MAIL_RU_CLOUD_H

#ifdef __cplusplus
extern "C" {
#endif

struct CURL;
struct interface;

// MailRuCloud holds all information which required for the api operations.
struct mail_ru_cloud {
	struct CURL *curl;
	char *auth_token;
	struct {
		char *get;
		char *upload;
	} shard;
	void (*io_progress)(int64_t, struct interface *interface);
	struct interface *(*init_io_progress)(int64_t);
};

struct mail_ru_cloud *new_cloud(const char *user,
				const char *password,
				const char *domain,
				int *error);
void delete_cloud(struct mail_ru_cloud *c);

int get_shard_info(struct mail_ru_cloud *c);

#ifdef __cplusplus
}
#endif

#endif /* __MAIL_RU_CLOUD_H */
