#include "terminal/terminal.h"

#include <unistd.h> /* Gives us read() and STDIN_FILENO (the keyboard input stream). */

/* The byte the terminal sends when the user presses Ctrl-Q.
 * 0x11 is hexadecimal for 17, which is Ctrl-Q's ASCII control code.
 * A #define is a text replacement: everywhere CONTROL_Q appears, the compiler
 * substitutes 0x11. Using a name instead of a bare number makes it readable. */
#define CONTROL_Q 0x11

/*
 * The main loop of the editor.
 * It keeps reading one key at a time and stops when the user asks to quit.
 *
 * Parameters:
 *   none.
 *
 * Returns:
 *   void: nothing. It simply stops when the quit condition is met.
 */
static void application_event_loop(void)
{
    int running = 1;

    while (running)
    {
        /* One keystroke is a single byte. unsigned char holds 0..255, which is
         * the full range of byte values we can read. Declared inside the loop
         * so it is freshly created on every pass. */
        unsigned char input;

        /* read() waits for input, then copies up to sizeof(input) bytes into
         * &input (its address) and returns how many bytes it actually read.
         * ssize_t is a signed size type: it can be negative on error (-1).
         * const means we will not modify bytes_read after this line. */
        const ssize_t bytes_read = read(STDIN_FILENO, &input, sizeof(input));

        if (bytes_read == 1 && input == CONTROL_Q)
        {
            running = 0;
        }
        else if (bytes_read == 0)
        {
            running = 0;
        }
    }
}

/*
 * Program entry point. The OS calls this to start the program.
 *
 * Parameters:
 *   none.
 *
 * Returns:
 *   int: the exit status. 0 means success; any non-zero value means failure.
 */
int main(void)
{
    TerminalState terminal;

    if (!terminal_init(&terminal))
    {
        return 1;
    }

    if (!terminal_enable_raw_mode(&terminal))
    {
        terminal_restore(&terminal);
        return 1;
    }

    application_event_loop();

    terminal_restore(&terminal);

    return 0;
}
