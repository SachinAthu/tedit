#define _POSIX_C_SOURCE 200809L

#include "common/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#ifndef TEDIT_VERSION
#define TEDIT_VERSION "unknown"
#endif

static FILE *logger_stream = NULL;
static LoggerLevel logger_minimum_level = LOGGER_LEVEL_INFO;
static bool logger_write_failed = false;

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

bool logger_init(const char *path, LoggerLevel minimum_level)
{
    if (logger_stream != NULL)
    {
        logger_shutdown();
    }

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

    va_start(arguments, format);
    result = vfprintf(logger_stream, format, arguments);
    va_end(arguments);
    if (result < 0 || fputc('\n', logger_stream) == EOF)
    {
        logger_write_failed = true;
    }
}

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
