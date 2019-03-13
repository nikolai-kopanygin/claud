#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <claud/utils.h>
#include <claud/types.h>
#include <claud/cld.h>
#include "command.h"

struct file_list;

static const char* ERROR_FILE_NOT_SPEC = "File not specified\n";
static const char* ERROR_DIR_NOT_SPEC = "Directory not specified\n";
static const char* ERROR_FILE_DIR_NOT_SPEC = "File or direcory not specified\n";
static const char* ERROR_FEW_ARGS = "Not enough arguments\n";
static const char* ERROR_ALLOC_ARGS = "Could not allocate memory for arguments\n";
static const char* ERROR_WRONG_CMD = "Wrong command\n";

static int print_file_list_item(struct list_item *li)
{
	struct tm tm = { 0 };
	char time_str[80];

	if (!li)
		return 1;
	
	if (!gmtime_r(&li->mtime, &tm)) {
		log_error("Wrong time format\n");
		return 1;
	}

	if (!strftime(time_str, sizeof(time_str), "%F %T", &tm)) {
		log_error("Wrong time format\n");
		return 1;
	}	
	
	printf("%-6s %11ld %-12s %-20s\n",
		li->kind,
		li->size,
		time_str,
		li->name);
	return 0;
}

static int print_file_list(struct file_list *finfo)
{
	size_t i;
	for (i = 0; i < finfo->body.nr_list_items; i++)
		if (print_file_list_item(&finfo->body.list[i]))
			return 1;
	return 0;	
}

static int command_print_file_list(struct command *cmd)
{
	int res;
	const char *path = cmd->args[0];
	struct file_list finfo = { 0 };
	if (cld_get_file_list(cmd->cld, path, &finfo, cmd->raw)) {
		log_error("Could not read file list\n");
		return 1;
	}
	res = print_file_list(&finfo);
	cld_file_list_cleanup(&finfo);
	return res;
}

static int print_file_stat(struct file_list *finfo)
{
	struct tm tm = { 0 };
	char time_str[80];

	if (!finfo)
		return 1;
	
	if (!gmtime_r(&finfo->body.mtime, &tm)) {
		log_error("Wrong time format\n");
		return 1;
	}

	if (!strftime(time_str, sizeof(time_str), "%F %T", &tm)) {
		log_error("Wrong time format\n");
		return 1;
	}	
	
	printf("%-6s %11ld %-12s %-20s hash:%s\n",
		finfo->body.kind,
		finfo->body.size,
		time_str,
		finfo->body.name,
		finfo->body.hash);
}

static int command_stat(struct command *cmd)
{
	int res;
	struct file_list finfo = { 0 };
	if (cld_file_stat(cmd->cld, cmd->args[0], &finfo)) {
		log_error("Could not read file info\n");
		return 1;
	}
	res = print_file_stat(&finfo);
	cld_file_list_cleanup(&finfo);
	return res;
}

static int command_share(struct command *cmd)
{
	int res;
	char *link;
	res = cld_share(cmd->cld, cmd->args[0], &link);
	if (res)
		log_error("Could not share file link\n");
	else
		printf("%s\n", link);
	free(link);
	return res;
}

static int command_remove(struct command *cmd)
{
	return cld_remove(cmd->cld, cmd->args[0]);
}

static int command_mkdir(struct command *cmd)
{
	return cld_mkdir(cmd->cld, cmd->args[0]);
}

/**
 * Cat file at mail.ru cloud.
/* cmd->args[0] is the full file path.
 */
static int command_cat(struct command *cmd)
{
	return cld_get_part(cmd->cld, STDOUT_FILENO, cmd->args[0]);
}

static int command_get(struct command *cmd)
{
	return cld_get(cmd->cld, cmd->args[0], cmd->args[1]);
}

static int command_upload(struct command *cmd)
{
	return cld_upload(cmd->cld, cmd->args[0], cmd->args[1]);
}

static int command_move(struct command *cmd)
{
	return cld_move(cmd->cld, cmd->args[0], cmd->args[1]);
}

static int command_copy(struct command *cmd)
{
	return cld_copy(cmd->cld, cmd->args[0], cmd->args[1]);
}

static int command_space(struct command *cmd)
{
	struct space_info info = { 0 };
	int res = cld_df(cmd->cld, &info);
	printf("Total: %ld bytes\nUsed: %ld bytes\nOverquota: %s\n",
		info.body.bytes_total, info.body.bytes_used,
		info.body.overquota ? "true" : "false"
      	);
}

static int parse_args(struct command *cmd, char *args[], int nr_args,
		      const char *msg_few_args)
{
	int i = 0;
	if (nr_args <= cmd->nr_args) {
		log_error("%s", msg_few_args);
		return 1;
	}
	for (; i < cmd->nr_args; i++) {
		cmd->args[i] = strdup(args[i+1]);
		if (!cmd->args[i]) {
			log_error("%s", ERROR_ALLOC_ARGS);
			return 1;
		}
	}
	return 0;
}

static int parse_args_ls(struct command *cmd, char *args[], size_t nr_args)
{
	cmd->args[0] = strdup(nr_args < 2 ? "/" : args[1]);
	if (!cmd->args[0]) {
		log_error("%s", ERROR_ALLOC_ARGS);
		return 1;
	}
	return 0;
}

static int parse_args_get(struct command *cmd, char *args[], size_t nr_args)
{
	if (nr_args < 2) {
		log_error("%s", ERROR_FILE_NOT_SPEC);
		return 1;
	}
	cmd->args[0] = strdup(args[1]);
	cmd->args[1] = nr_args == 2 ? copy_basename(args[1]) : strdup(args[2]);
	if (!cmd->args[0] || !cmd->args[1]) {
		log_error("%s", ERROR_ALLOC_ARGS);
		return 1;
	}
	return 0;
}

static int parse_args_put(struct command *cmd, char *args[], size_t nr_args)
{
	if (nr_args < 2) {
		log_error("%s", ERROR_FILE_NOT_SPEC);
		return 1;
	}
	cmd->args[0] = strdup(args[1]);
	if (nr_args == 2) {
		char *tmp = copy_basename(args[1]);
		if (!tmp)
			return 1;
		cmd->args[1] = (char *)malloc(strlen("/") + strlen(tmp) + 1);
		if (cmd->args[1])
			sprintf(cmd->args[1], "/%s", tmp);
		free(tmp);
	} else {
		cmd->args[1] = strdup(args[2]);
	}
	if (!cmd->args[0] || !cmd->args[1]) {
		log_error("%s", ERROR_ALLOC_ARGS);
		return 1;
	}
	return 0;
}

/**
 * Parse the parameters of the program and fill in the command descriptor.
 * @param cmd - the command descriptor;
 * @param args - the parameters;
 * @param nr_args - the number of parameters;
 * @return 0 for success, or error code.
 */
int command_parse(struct command *cmd, char *args[], size_t nr_args)
{
	int err = 0;
	char *op = args[0];
	if (!strcmp(op, "ls")) {
		cmd->handle = command_print_file_list;
		cmd->nr_args = 1;
		err = parse_args_ls(cmd, args, nr_args);
	} else if (!strcmp(op, "rm")) {
		cmd->handle = command_remove;
		cmd->nr_args = 1;
		err = parse_args(cmd, args, nr_args, ERROR_FILE_DIR_NOT_SPEC);
	} else if (!strcmp(op, "mkdir")) {
		cmd->handle = command_mkdir;
		cmd->nr_args = 1;
		err = parse_args(cmd, args, nr_args, ERROR_DIR_NOT_SPEC);
	} else if (!strcmp(op, "share")) {
		cmd->handle = command_share;
		cmd->nr_args = 1;
		err = parse_args(cmd, args, nr_args, ERROR_FILE_DIR_NOT_SPEC);
	} else if (!strcmp(op, "stat")) {
		cmd->handle = command_stat;
		cmd->nr_args = 1;
		err = parse_args(cmd, args, nr_args, ERROR_FILE_DIR_NOT_SPEC);
	} else if (!strcmp(op, "cat")) {
		cmd->handle = command_cat;
		cmd->nr_args = 1;
		err = parse_args(cmd, args, nr_args, ERROR_FILE_NOT_SPEC);
	} else if (!strcmp(op, "get")) {
		cmd->handle = command_get;
		cmd->nr_args = 2;
		err = parse_args_get(cmd, args, nr_args);
	} else if (!strcmp(op, "put")) {
		cmd->handle = command_upload;
		cmd->nr_args = 2;
		err = parse_args_put(cmd, args, nr_args);
	} else if (!strcmp(op, "mv")) {
		cmd->handle = command_move;
		cmd->nr_args = 2;
		err = parse_args(cmd, args, nr_args, ERROR_FEW_ARGS);
	} else if (!strcmp(op, "cp")) {
		cmd->handle = command_copy;
		cmd->nr_args = 2;
		err = parse_args(cmd, args, nr_args, ERROR_FEW_ARGS);
	} else if (!strcmp(op, "df")) {
		cmd->handle = command_space;
		cmd->nr_args = 0;
		err = parse_args(cmd, args, nr_args, ERROR_FEW_ARGS);
	} else{
		log_error("%s", ERROR_WRONG_CMD);
		err = 1;
	}
	if (err)
		command_cleanup(cmd);
	return err;
}

/**
 * Free the resources allocated for the command.
 * @param cmd - the command descriptor.
 */
void command_cleanup(struct command *cmd)
{
	int i;
	for (i = 0; i < cmd->nr_args; i++)
		free(cmd->args[i]);
	memset(cmd, 0, sizeof(cmd));
}

