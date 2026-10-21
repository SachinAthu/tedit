#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H

#include <stdbool.h>

/*
 * Severity of a diagnostic record, ordered from most to least severe.
 *
 * A record is written only when its level is numerically at or below the
 * session's configured minimum, so ERROR (0) is always recorded while DEBUG
 * is the first level to be filtered out as the minimum level rises.
 */
typedef enum
{
    LOGGER_LEVEL_ERROR = 0,
    LOGGER_LEVEL_WARN,
    LOGGER_LEVEL_INFO,
    LOGGER_LEVEL_DEBUG
} LoggerLevel;

/*
 * Open a diagnostic log file for the current session, replacing any records
 * left by a previous session.
 *
 * A "session start" record is written immediately, naming the build version
 * and process ID so separate runs can be told apart in the file.
 *
 * Logging is deliberately best-effort: if the file cannot be opened, the
 * editor still starts normally and later log calls become harmless no-ops.
 *
 * Parameters:
 *   path          - filesystem path of the log file; must not be NULL or
 *                   empty.
 *   minimum_level - the least-severe level that is still recorded; levels
 *                   numerically greater than this are ignored.
 *
 * Returns:
 *   true if the log was opened and the session-start record written, false
 *   on invalid arguments or if the file could not be opened.
 */
bool logger_init(const char *path, LoggerLevel minimum_level);

/*
 * Write one formatted diagnostic record.
 *
 * This is the single sink for all log output. It uses printf-style format
 * strings plus variadic arguments, so a NULL or empty format is rejected
 * rather than passed on.
 *
 * Logging is best-effort: any write failure latches a "write failed" flag
 * and silently disables further output for the rest of the session, so a
 * full disk or closed stream can never crash the editor.
 *
 * Parameters:
 *   level     - severity of the record; ignored when above the minimum level.
 *   component - subsystem name to attribute the record to; NULL or empty is
 *               replaced with "unknown".
 *   format    - printf-style format string; must not be NULL.
 *   ...       - values to substitute into `format`, as in printf().
 *
 * Returns:
 *   void.
 */
void logger_log(LoggerLevel level, const char *component, const char *format, ...);

/*
 * Flush and close the log file.
 *
 * Safe to call repeatedly, and safe to call when logging was never
 * initialized or when opening failed; it simply does nothing in those cases.
 */
void logger_shutdown(void);

/*
 * Return the stable, human-readable name used for a level in log records.
 *
 * Parameters:
 *   level - the level to name.
 *
 * Returns:
 *   the level's name, or "UNKNOWN" for an out-of-range value.
 */
const char *logger_level_name(LoggerLevel level);

/*
 * Convenience macros that call logger_log() with a fixed severity.
 *
 * They accept the same printf-style arguments as logger_log(). When logging
 * is disabled (TEDIT_ENABLE_LOGGING is not defined) they expand to nothing,
 * so diagnostic code that ships enabled-by-default costs nothing in a
 * release build.
 */
#ifdef TEDIT_ENABLE_LOGGING
#define TEDIT_LOG_ERROR(component, ...) \
    logger_log(LOGGER_LEVEL_ERROR, (component), __VA_ARGS__)
#define TEDIT_LOG_WARN(component, ...) \
    logger_log(LOGGER_LEVEL_WARN, (component), __VA_ARGS__)
#define TEDIT_LOG_INFO(component, ...) \
    logger_log(LOGGER_LEVEL_INFO, (component), __VA_ARGS__)
#define TEDIT_LOG_DEBUG(component, ...) \
    logger_log(LOGGER_LEVEL_DEBUG, (component), __VA_ARGS__)
#else
#define TEDIT_LOG_ERROR(component, ...) ((void)0)
#define TEDIT_LOG_WARN(component, ...) ((void)0)
#define TEDIT_LOG_INFO(component, ...) ((void)0)
#define TEDIT_LOG_DEBUG(component, ...) ((void)0)
#endif

#endif
