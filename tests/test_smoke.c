#include <assert.h>

/*
 * Smoke test: verifies the test harness itself can build and run. A "smoke
 * test" is a minimal check that the basic machinery works before running the
 * full suite. Here we simply assert an always-true statement.
 */
int main(void)
{
    assert(1 == 1);

    return 0;
}
