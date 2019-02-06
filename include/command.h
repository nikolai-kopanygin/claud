/**
 * @file command.h
 * API for Mail.Ru Cloud commands.
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

#ifndef __COMMAND_H
#define __COMMAND_H

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cld;
struct file_list;
struct command;

/** Command handler function type */
typedef int(cmd_handler_fn)(struct command *cmd);

/** Maximum number of arguments in a command */
#define CMD_MAX_ARGS 2

/** This is a descriptor for the Mail.Ru Cloud command */
struct command {
	struct cld *cld;	/**< Cloud ptr */
	cmd_handler_fn *handle;		/**< Handler ptr */
	char *args[CMD_MAX_ARGS];	/**< Command arguments */
	size_t nr_args;			/**< Number of command arguments */
	int err;			/**< Error code */
	bool progress;			/**< Flag: show progress */
	bool raw;			/**< Flag: operate on parts of split files */
};

/**
 * Parse the parameters of the program and fill in the command descriptor.
 * @param cmd - the command descriptor;
 * @param args - the parameters;
 * @param nr_args - the number of parameters;
 * @return 0 for success, or error code.
 */
int command_parse(struct command *cmd, char *args[], size_t nr_args);

/**
 * Free the resources allocated for the command.
 * @param cmd - the command descriptor.
 */
void command_cleanup(struct command *cmd);

int file_stat(struct cld *c, const char *path, struct file_list *finfo);
void file_list_cleanup(struct file_list *finfo);

#ifdef __cplusplus
}
#endif

#endif /* __COMMAND_H */
