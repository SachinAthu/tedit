#include "diagnostics.h"

#ifdef TEDIT_ENABLE_DIAGNOSTICS
#include <stdio.h>

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

    (void)fprintf(stderr, "[tedit:error][%s] %s\n", component, message);
}
#else
typedef int tedit_diagnostics_translation_unit;
#endif
