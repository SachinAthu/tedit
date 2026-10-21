#include "diagnostics.h"

#include "common/logger.h"

#include <stddef.h>

#ifdef TEDIT_ENABLE_DIAGNOSTICS
/*
 * Write one error-level diagnostic record through the shared logger.
 *
 * This exists so diagnostics go through the same sink as ordinary logging
 * rather than a competing output path. NULL arguments are replaced with
 * safe placeholder text.
 *
 * Parameters:
 *   component - the subsystem name to attribute the record to; may be NULL.
 *   message   - the diagnostic text; may be NULL.
 */
void tedit_log_error(const char *component, const char *message)
{
    if (component == NULL)
    {
        component = "unknown";
    }
    if (message == NULL)
    {
        message = "unspecified diagnostic";
    }

    logger_log(LOGGER_LEVEL_ERROR, component, "%s", message);
}
#else
typedef int tedit_diagnostics_translation_unit;
#endif
