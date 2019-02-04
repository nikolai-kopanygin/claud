#include <stdbool.h>
#include <sys/types.h>
struct mail_ru_cloud;
struct file_list;

#define CMD_MAX_ARGS 2

struct command {
	int (*handle)(struct mail_ru_cloud *c, struct command *cmd);
	char *args[CMD_MAX_ARGS];
	size_t nr_args;
	int err;
	bool progress;
	bool raw;
};

int command_parse(struct command *cmd, char *args[], size_t nr_args);
void command_cleanup(struct command *cmd);

int command_print_file_list(struct mail_ru_cloud *c, struct command *cmd);
int command_stat(struct mail_ru_cloud *c, struct command *cmd);
int command_remove(struct mail_ru_cloud *c, struct command *cmd);
int command_mkdir(struct mail_ru_cloud *c, struct command *cmd);
int command_cat(struct mail_ru_cloud *c, struct command *cmd);
int command_get(struct mail_ru_cloud *c, struct command *cmd);
int command_upload(struct mail_ru_cloud *c, struct command *cmd);
int command_move(struct mail_ru_cloud *c, struct command *cmd);
int command_copy(struct mail_ru_cloud *c, struct command *cmd);
int file_stat(struct mail_ru_cloud *c, const char *path, struct file_list *finfo);
void file_list_cleanup(struct file_list *finfo);
