#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H

#include <stdbool.h>

typedef enum
{
    LOGGER_LEVEL_ERROR = 0,
    LOGGER_LEVEL_WARN,
    LOGGER_LEVEL_INFO,
    LOGGER_LEVEL_DEBUG
} LoggerLevel;

/*
 * Open a diagnostic log for the current session, replacing any records left by
 * a previous session. A session-start record names the build and process.
 * Failure disables logging but must not prevent the editor from starting or
 * continuing. Returns true on success.
 */
bool logger_init(const char *path, LoggerLevel minimum_level);

/* Write one formatted diagnostic record. Logging is a best-effort observer. */
void logger_log(LoggerLevel level, const char *component, const char *format, ...);

/* Flush and close the log. Safe to call repeatedly. */
void logger_shutdown(void);

/* Return the stable name used in log records; unknown levels become "UNKNOWN". */
const char *logger_level_name(LoggerLevel level);

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
