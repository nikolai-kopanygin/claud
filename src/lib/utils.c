#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <claud/utils.h>
#include <claud/types.h>
#include <claud/http_api.h>

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
 * Extract the basename of file from its pathname
 * and copy it to a newly allocated string.
 * @param pathname - the file pathname;
 * @return the basename string.
 */
char *copy_basename(const char *pathname)
{
	char *filename;
	char *tmp = xstrdup(pathname);
	filename = xstrdup(basename(tmp));
	free(tmp);
	
	return filename;
}

/**
 * Extract the dirname of file from its pathname
 * and copy it to a newly allocated string.
 * @param pathname - the file pathname;
 * @return the dirname string.
 */
char *copy_dirname(const char *pathname)
{
	char *filename;
	char *tmp = xstrdup(pathname);
	char *dir = dirname(tmp);
	filename = xstrdup(strcmp(dir, ".") ? dir : "/");
	free(tmp);
	return filename;
}

/**
 * Merge dirname with basename to get the full path to a file.
 * Allocate a string and put the result in it.
 * @param dir - the dirname;
 * @param base - the basename.
 * @return pointer to allocated string, or NULL for out of memory error.
 */
char *join_dir_and_base(const char *dir, const char *base)
{
	char *s = xmalloc(strlen(dir) + strlen(base) + 2);
	if (dir[0] == '\0' || !strcmp(dir, "/"))
		strcpy(s, base);
	else
		sprintf(s, "%s/%s", dir, base);
	return s;
}

/**
 * Allocate a string big enough to include both the directory name, a slash
 * and the longest file name in this directory. Copy the directory name to
 * that string along with a trailing slash.
 * @param dirname - the directory name;
 * @param contents - the contents of the directory.
 * @return the pointer to the allocated string.
 */
char *extend_dirname(const char *dirname, struct file_list *contents)
{
	char *full_path;
	size_t filename_len = 0; /* maximum for this dir */
	size_t dirname_len = strlen(dirname);
	size_t i = 0;
	struct list_item *list = contents->body.list;
	size_t nr_items = contents->body.nr_list_items;

	for (; i < nr_items; i++)
		if (filename_len < strlen(list[i].name))
			filename_len = strlen(list[i].name);
		
	if (dirname[dirname_len - 1] == '/')
		dirname_len -= 1;
	
	full_path = xmalloc(dirname_len + filename_len + 2);
	strncpy(full_path, dirname, dirname_len);
	strcpy(full_path + dirname_len, "/");
	
	return full_path;
}

/**
 * Fill the string of a specified length with random
 * alphanumeric data.
 * @param s - the pointer to the string;
 * @param len - the length of the string.
 */
void fill_random(char *s, const size_t len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	size_t i = 0;

	for (; i < len - 1; i++)
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	s[len - 1] = 0;
}

/**
 * Create a file part name out of file name and part index.
 * @param name - the file name;
 * @param idx - the part index.
 * @return - a pointer to string containing the part name.
 */
char *get_file_part_name(const char *name, int idx)
{
	static const char *sfx = PART_SUFFIX;
	static const char *fmt = PART_FORMAT;
	int n = strlen(name) + strlen(sfx) + 3;
	char *str = NULL;
	
	while (1) {
		char *new_str = xrealloc(str, n);
		int slen = snprintf(new_str, n, fmt, name, sfx, idx);
		if (slen < n)
			/* All OK */
			return new_str;
		str = new_str;
		n = slen + 1;
	}
	
	log_error("We should never come here!\n");
	exit(1);
}
