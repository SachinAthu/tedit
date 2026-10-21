#define _POSIX_C_SOURCE 200809L

#include "common/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

/*
 * Fallback build identifier used when the build system did not supply one.
 * It appears in the session-start record written by logger_init().
 */
#ifndef TEDIT_VERSION
#define TEDIT_VERSION "unknown"
#endif

/*
 * Module state. Logging keeps a single open FILE for the whole session plus
 * two pieces of bookkeeping: the minimum severity that is still recorded and
 * a latch that disables further output once any write has failed. These are
 * file-scoped (static), so the rest of the program never touches them
 * directly - it goes through logger_log() and friends.
 */
static FILE *logger_stream = NULL;
static LoggerLevel logger_minimum_level = LOGGER_LEVEL_INFO;
static bool logger_write_failed = false;

/*
 * Return the stable, human-readable name used for a level in log records.
 *
 * Parameters:
 *   level - the level to name.
 *
 * Returns:
 *   the level's name, or "UNKNOWN" for an out-of-range value.
 */
const char *logger_level_name(LoggerLevel level)
{
    switch (level)
    {
    case LOGGER_LEVEL_ERROR:
        return "ERROR";
    case LOGGER_LEVEL_WARN:
        return "WARN";
    case LOGGER_LEVEL_INFO:
        return "INFO";
    case LOGGER_LEVEL_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

/*
 * Open a diagnostic log file for the current session, replacing any records
 * left by a previous session. See logger.h for the full contract.
 *
 * If a previous session's log is still open, it is closed first so opening
 * twice never leaks a FILE handle. The open path is then validated, the file
 * is opened in write mode (truncating any old content), and buffering is
 * enabled so the file is not flushed on every record.
 *
 * Parameters:
 *   path          - filesystem path of the log file; must not be NULL or
 *                   empty.
 *   minimum_level - the least-severe level that is still recorded.
 *
 * Returns:
 *   true if the log was opened and the session-start record written, false
 *   on invalid arguments or if the file could not be opened or buffered.
 */
bool logger_init(const char *path, LoggerLevel minimum_level)
{
    if (logger_stream != NULL)
    {
        logger_shutdown();
    }

    /* Start clean: no open stream, no latched failure. */
    logger_stream = NULL;
    logger_write_failed = false;

    if (path == NULL || path[0] == '\0' || minimum_level < LOGGER_LEVEL_ERROR || minimum_level > LOGGER_LEVEL_DEBUG)
    {
        return false;
    }

    logger_stream = fopen(path, "w");
    if (logger_stream == NULL)
    {
        return false;
    }

    logger_minimum_level = minimum_level;
    /* Buffered writes avoid flushing the file for every input event. */
    if (setvbuf(logger_stream, NULL, _IOFBF, BUFSIZ) != 0)
    {
        (void)fclose(logger_stream);
        logger_stream = NULL;
        return false;
    }

    /* The record timestamp is the session start time; version and pid make
     * the log attributable and distinguish one run from another. */
    logger_log(LOGGER_LEVEL_INFO, "logger", "session start version=%s pid=%ld",
               TEDIT_VERSION, (long)getpid());

    return true;
}

/*
 * Write one formatted diagnostic record. See logger.h for the full contract.
 *
 * Records are dropped early when there is no open stream, a previous write
 * already failed, the level is filtered out, or the format string is NULL.
 * Otherwise a timestamp, level name, and component are printed, followed by
 * the caller's formatted text and a trailing newline.
 *
 * Parameters:
 *   level     - severity of the record.
 *   component - subsystem name to attribute the record to.
 *   format    - printf-style format string; must not be NULL.
 *   ...       - values to substitute into `format`, as in printf().
 *
 * Returns:
 *   void.
 */
void logger_log(LoggerLevel level, const char *component, const char *format, ...)
{
    time_t now;
    struct tm local_time;
    char timestamp[32];
    va_list arguments;
    int result;

    if (logger_stream == NULL || logger_write_failed ||
        level < LOGGER_LEVEL_ERROR || level > LOGGER_LEVEL_DEBUG ||
        level > logger_minimum_level || format == NULL)
    {
        return;
    }

    /* Build a human-readable timestamp, falling back to a placeholder if the
     * clock or locale conversion is unavailable. */
    now = time(NULL);
    if (now == (time_t)-1 || localtime_r(&now, &local_time) == NULL || strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time) == 0)
    {
        (void)snprintf(timestamp, sizeof(timestamp), "time-unavailable");
    }

    if (component == NULL || component[0] == '\0')
    {
        component = "unknown";
    }

    result = fprintf(logger_stream, "[%s] [%s] [%s] ", timestamp, logger_level_name(level), component);
    if (result < 0)
    {
        logger_write_failed = true;
        return;
    }

    /* vfprintf() formats the caller's variadic arguments; it is the varargs
     * counterpart of fprintf(). */
    va_start(arguments, format);
    result = vfprintf(logger_stream, format, arguments);
    va_end(arguments);
    if (result < 0 || fputc('\n', logger_stream) == EOF)
    {
        logger_write_failed = true;
    }
}

/*
 * Flush and close the log file.
 *
 * Safe to call repeatedly and to call when logging was never initialized or
 * when opening failed; it does nothing in those cases. The pending-buffer
 * flush is skipped after a write failure, since there is no point forcing
 * known-bad data out.
 */
void logger_shutdown(void)
{
    if (logger_stream == NULL)
    {
        return;
    }

    if (!logger_write_failed)
    {
        (void)fflush(logger_stream);
    }
    (void)fclose(logger_stream);
    logger_stream = NULL;
    logger_write_failed = false;
}
