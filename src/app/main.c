/**
 * @file claud.c
 * A simple Mail.Ru Cloud client written in C language.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "cld.h"
#include "command.h"
#include "utils.h"

const char *program_name;

void help(FILE *f)
{
	fprintf(f, "%s is a simple Mail.ru Cloud Client\n", program_name);
	fprintf(f, "Usage: %s [OPTIONS] <COMMAND> [FILE/DIR] [FILE/DIR]\n", program_name);
	fprintf(f, "Provides basic operations with files stored at Mail.ru Cloud\n\n");
	fprintf(f, "Options:\n"
		"  -h, --help                   Print this help message\n"
		"  -v, --verbose                Level of verbosity (0-3)\n"
		"\n");
	fprintf(f, "Commands := < cp | cat | get | ls | mkdir | mv | put | rm | stat >\n\n");
	fprintf(f, "Example: %s ls\n", program_name);
}

void usage()
{
	help(stderr);
	exit(1);
}


int main(int argc, char *argv[])
{
	program_name = argv[0];
	int index;
	int err;
	struct command cmd = { 0, };
	struct cld *c;
	
	/* parse options */
	static struct option loptions[] = {
		{"help", 0, 0,'h'},
		{"verbose", 0, 0,'v'},
		{"progress", 0, 0,'p'},
		{"raw", 0, 0,'r'},
		{0,0,0,0}
	};
	
	init_log(stderr);
	
	while(1) {
		int option_index = 0;
		int opt = getopt_long (argc, argv, "hprv:", 
			loptions, &option_index);
		if (opt==-1) break;
	
		switch (opt) {
		case 'h':
			help(stdout);
			exit(0);
			break;
		case 'p':
			cmd.progress = true;
			break;
		case 'r':
			cmd.raw = true;
			break;
		case 'v':
			set_log_level(atoi(optarg));
			break;
		default:
			usage();
		}
	}
	
	if (optind == argc) {
		usage();
	}

	if (command_parse(&cmd, argv + optind, argc - optind)) {
		usage();
	}
	
	curl_global_init(CURL_GLOBAL_ALL);
	c = new_cloud(getenv("MAILRU_USER"), getenv("MAILRU_PASSWORD"),
		      "mail.ru", &err);
	if (c) {
		cmd.cld = c;
		cmd.handle(&cmd);
		err = cmd.err;
		delete_cloud(c);
	} else {
		log_error("new_cloud failed\n");
	}
	command_cleanup(&cmd);
	curl_global_cleanup();
	return err;
}

