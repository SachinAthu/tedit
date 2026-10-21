#include "diagnostics.h"

#include "common/logger.h"

#include <stddef.h>

#ifdef TEDIT_ENABLE_DIAGNOSTICS
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
