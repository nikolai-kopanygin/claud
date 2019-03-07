#ifndef __UTILS_H
#define __UTILS_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

struct file_list;
	
/* Logging */

/**
 * Severity levels of log messages
 */
enum log_levels {
	LOG_ERROR = 0,	/**< Error */
	LOG_WARN,	/**< Warning */
	LOG_INFO,	/**< Information */
	LOG_DEBUG	/**< Debugging */
};

/**
 * Initialize the logger with a file pointer (e.g., stderr).
 * @param fp - the file pointer for logging.
 */
void init_log(FILE *fp);

/**
 * Generic log message function, use with warpper macros.
 * @param level - the message severity level;
 * @param file - the source file name;
 * @param line - the source line number;
 * @param fmt - the message format.
 * @return Nothing
 */
void log_message(int level, const char *file, int line, const char *fmt, ...);

/** Use for error messages */
#define log_error(...) log_message(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

/** Use for warnings */
#define log_warn(...) log_message(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)

/** Use for info messages */
#define log_info(...) log_message(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

/** Use for debug messages */
#define log_debug(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

/**
 * Set the log_level to the specified value with range checking.
 * @param val - the new value.
 * @return Nothing
 */
void set_log_level(int val);

/**
 * Get the log_level value.
 * @return the value.
 */
int get_log_level();

/**
 * Extract the basename of  file from its pathname
 * and copy it to a newly allocated string.
 * @param pathname - the file pathname;
 * @return the basename string, or NULL if allocation failed.
 */
char *copy_basename(const char *pathname);

/**
 * Extract the dirname of file from its pathname
 * and copy it to a newly allocated string.
 * @param pathname - the file pathname;
 * @return the dirname string, or NULL if allocation failed.
 */
char *copy_dirname(const char *pathname);

/**
 * Merge dirname with basename to get the full path to a file.
 * Allocate a string and put the result in it.
 * @param dir - the dirname;
 * @param base - the basename.
 * @return pointer to allocated string, or NULL for out of memory error.
 */
char *join_dir_and_base(const char *dir, const char *base);

/**
 * Allocate a string big enough to include both the directory name, a slash
 * and the longest file name in this directory. Copy the directory name to
 * that string along with a trailing slash.
 * @param dirname - the directory name;
 * @param contents - the contents of the directory.
 * @return the pointer to the allocated string, or NULL for error.
 */
char *extend_dirname(const char *dirname, struct file_list *contents);

/**
 * Strip whitespace off of both ends of a string.
 * NOTE: the original string is modified.
 * @param str - the string to trim.
 * @return - a pointer to the trimmed string.
 */
char *trimwhitespace(char *str);


/**
 * Create a file part name out of file name and part index.
 * @param name - the file name;
 * @part idx - the part index.
 * @return - a pointer to string containing the part name.
 */
char *get_file_part_name(const char *name, int idx);

/**
 * Fill the string of a specified length with random
 * alphanumeric data.
 * @param s - the pointer to the string;
 * @param len - the length of the string.
 */
void fill_random(char *s, const size_t len);

/**
 * Malloc wrapper terminating the program when memory runs up.
 * @param size - the requested allocation size.
 * @return the pointer to the allocated memory.
 */
inline static void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr) {
		log_error("Out of memory\n");
		exit(1);
	}
	return ptr;
}

/**
 * Calloc wrapper terminating the program when memory runs up.
 * @param nmemb - number of elements to allocate;
 * @param size - the element size.
 * @return the pointer to the allocated memory.
 */
inline static void *xcalloc(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	if (!ptr) {
		log_error("Out of memory\n");
		exit(1);
	}
	return ptr;
}

/**
 * Realloc wrapper terminating the program when memory runs up.
 * @param ptr - the pointer to the memory block;
 * @param size - the requested reallocation size.
 * @return the pointer to the allocated memory.
 */
inline static void *xrealloc(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr, size);
	if (!new_ptr && size > 0) {
		log_error("Out of memory\n");
		exit(1);
	}
	return new_ptr;
}

/**
 * Strdup wrapper terminating the program when memory runs up.
 * @param s - the source string to copy.
 * @return the pointer to the allocated string.
 */
inline static char *xstrdup(const char *s)
{
	void *new_s = strdup(s);
	if (!new_s) {
		log_error("Out of memory\n");
		exit(1);
	}
	return new_s;
}

/**
 * Strndup wrapper terminating the program when memory runs up.
 * @param s - the source string to copy;
 * @param n - the length of the source string.
 * @return the pointer to the allocated string.
 */
inline static char *xstrndup(const char *s, size_t n)
{
	void *new_s = strndup(s, n);
	if (!new_s) {
		log_error("Out of memory\n");
		exit(1);
	}
	return new_s;
}

#ifdef __cplusplus
}
#endif

#endif
