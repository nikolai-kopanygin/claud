#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "utils.h"

/**
 * Maximum log level of messages to be shown.
 */
static int log_level = LOG_ERROR;

/**
 * Extract the basename of  file from its pathname
 * and copy it to a newly allocated string.
 * @param pathname - the file pathname;
 * @return the basename string, or NULL if allocation failed.
 */
char *copy_basename(const char *pathname)
{
	char *filename;
	char *tmp = strdup(pathname);
	if (!tmp)
		return NULL;
	filename = strdup(basename(tmp));
	free(tmp);
	
	return filename;
}

static const char *level_names[] = { "ERROR", "WARN", "INFO", "DEBUG" };

/**
 * Generic log message function, use with warpper macros.
 * @param level - the message severity level;
 * @param file - the source file name;
 * @param line - the source line number;
 * @param fmt - the message format.
 * @return Nothing
 */
void log_message(int level, const char *file, int line, const char *fmt, ...)
{
	int n;
	int size = 100;     /* Guess we need no more than 100 bytes */
	char *p, *np;
	va_list ap;

	if (log_level < level || level > LOG_DEBUG || level < LOG_ERROR)
		return;
	
	if ((p = malloc(size)) == NULL)
		return;

	/* Try to print in the allocated space */
	while (1) {
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);

		/* Check error code */
		if (n < 0)
			return;

		/* If that worked, fine */
		if (n < size)
			break;

		/* Else try again with more space */
		size = n * 2;
		if ((np = realloc (p, size)) == NULL) {
			free(p);
			return;
		} else {
			p = np;
		}
	}

	fprintf(stderr, "%s [%s:%d] %s", level_names[level], file, line, p);
	free(p);
}

/**
 * Set the log_level to the specified value with range checking.
 * @param val - the new value.
 * @return Nothing
 */
void set_log_level(int val)
{
	if (val < LOG_ERROR || val > LOG_DEBUG)
		return;
	log_level = val;
}

/**
 * Get the log_level value.
 * @return the value.
 */
int get_log_level()
{
	return log_level;
}
