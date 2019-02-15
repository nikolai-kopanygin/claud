#ifndef __UTILS_H
#define __UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

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
 * Strip whitespace off of both ends of a string.
 * NOTE: the original string is modified.
 * @param str - the string to trim.
 * @return - a pointer to the trimmed string.
 */
char *trimwhitespace(char *str);

/**
 * Fill the string of a specified length with random
 * alphanumeric data.
 * @param s - the pointer to the string;
 * @param len - the length of the string.
 */
void fill_random(char *s, const size_t len);

#ifdef __cplusplus
}
#endif

#endif
