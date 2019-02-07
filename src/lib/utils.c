#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <claud/utils.h>

/**
 * Maximum log level of messages to be shown.
 */
static int log_level = LOG_ERROR;

static FILE *log_fp = NULL;

static const char *level_names[] = { "ERROR", "WARN", "INFO", "DEBUG" };

/**
 * Initialize the logger with a file pointer (e.g., stderr).
 * @param fp - the file pointer for logging.
 */
void init_log(FILE *fp)
{
	log_fp = fp;
}

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
	
	if (!log_fp)
		return;

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

	fprintf(log_fp, "%s [%s:%d] %s", level_names[level], file, line, p);
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

/**
 * Strip whitespace off of both ends of a string.
 * NOTE: the original string is modified.
 * @param str - the string to trim.
 * @return - a pointer to the trimmed string.
 */
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

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
