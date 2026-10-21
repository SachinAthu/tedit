#define _POSIX_C_SOURCE 200809L

#include "common/logger.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void read_log(const char *path, char *buffer, size_t capacity)
{
    FILE *stream = fopen(path, "r");
    assert(stream != NULL);
    size_t length = fread(buffer, 1, capacity - 1, stream);
    buffer[length] = '\0';
    assert(!ferror(stream));
    assert(fclose(stream) == 0);
}

int main(void)
{
    char path[] = "/tmp/tedit-logger-test-XXXXXX";
    char contents[4096];
    int descriptor = mkstemp(path);

    assert(descriptor >= 0);
    assert(close(descriptor) == 0);
    assert(unlink(path) == 0);

    assert(logger_init(path, LOGGER_LEVEL_INFO));
    logger_log(LOGGER_LEVEL_ERROR, "test", "file operation failed: %d", 7);
    logger_log(LOGGER_LEVEL_WARN, "test", "recoverable warning");
    logger_log(LOGGER_LEVEL_INFO, "test", "session started");
    logger_log(LOGGER_LEVEL_DEBUG, "test", "this record must be filtered");
    logger_shutdown();

    read_log(path, contents, sizeof(contents));
    assert(strstr(contents, "[ERROR] [test] file operation failed: 7") != NULL);
    assert(strstr(contents, "[WARN] [test] recoverable warning") != NULL);
    assert(strstr(contents, "[INFO] [test] session started") != NULL);
    assert(strstr(contents, "this record must be filtered") == NULL);
    assert(strstr(contents, "[20") != NULL); /* timestamp prefix */
    assert(strstr(contents, "[INFO] [logger] session start version=") != NULL);
    assert(strstr(contents, " pid=") != NULL);

    /* Reinitialization starts a new session, replacing previous records. */
    assert(logger_init(path, LOGGER_LEVEL_DEBUG));
    logger_log(LOGGER_LEVEL_DEBUG, "test", "debug enabled");
    logger_shutdown();
    read_log(path, contents, sizeof(contents));
    assert(strstr(contents, "session started") == NULL);
    assert(strstr(contents, "[INFO] [logger] session start version=") != NULL);
    assert(strstr(contents, "[DEBUG] [test] debug enabled") != NULL);

    /* An unavailable log path disables logging without making it fatal. */
    assert(!logger_init("/no-such-tedit-directory/log.txt", LOGGER_LEVEL_DEBUG));
    logger_log(LOGGER_LEVEL_ERROR, "test", "must safely become a no-op");
    logger_shutdown();

    /* Linux exposes /dev/full to deterministically reject writes. This check
     * is optional on other POSIX systems and verifies failure is non-fatal. */
    if (access("/dev/full", F_OK) == 0)
    {
        assert(logger_init("/dev/full", LOGGER_LEVEL_DEBUG));
        logger_log(LOGGER_LEVEL_ERROR, "test", "simulated log write failure");
        logger_shutdown();
    }

    assert(unlink(path) == 0);
    return 0;
}
