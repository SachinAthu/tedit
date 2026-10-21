#ifndef DIAGNOSTICS_DIAGNOSTICS_H
#define DIAGNOSTICS_DIAGNOSTICS_H

#include <assert.h>

/* Assertions check internal invariants; public input validation remains explicit. */
#define TEDIT_ASSERT(condition) assert(condition)

#ifdef TEDIT_ENABLE_DIAGNOSTICS
void tedit_log_error(const char *component, const char *message);
#define TEDIT_DIAGNOSTIC_ERROR(component, message) tedit_log_error((component), (message))
#else
#define TEDIT_DIAGNOSTIC_ERROR(component, message) ((void)0)
#endif

#endif
