#include "terminal/terminal.h"

#include <assert.h>
#include <stddef.h>

int main(void)
{
    size_t rows = 17;
    size_t columns = 29;

    assert(!terminal_get_size(NULL, &columns));
    assert(!terminal_get_size(&rows, NULL));
    assert(!terminal_get_size(NULL, NULL));
    assert(rows == 17);
    assert(columns == 29);

    /* Cleanup helpers must tolerate absent terminal state. */
    terminal_restore(NULL);
    terminal_leave_alternate_screen(NULL);

    return 0;
}
