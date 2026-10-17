#ifndef INPUT_INPUT_PARSER_H
#define INPUT_INPUT_PARSER_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The kind of key press the Input Parser recognized.
 *
 * The parser turns raw terminal bytes into one of these abstract events, so
 * the rest of the editor never has to know about terminal escape sequences
 * or UTF-8 encoding.
 */
typedef enum
{
    INPUT_EVENT_NONE = 0,  /* no complete key recognized yet */
    INPUT_EVENT_CHARACTER, /* a printable Unicode character */
    INPUT_EVENT_CTRL_Q,
    INPUT_EVENT_CTRL_S,
    INPUT_EVENT_BACKSPACE,
    INPUT_EVENT_ENTER,
    INPUT_EVENT_ESCAPE,
    INPUT_EVENT_ARROW_LEFT,
    INPUT_EVENT_ARROW_RIGHT,
    INPUT_EVENT_ARROW_UP,
    INPUT_EVENT_ARROW_DOWN,
    INPUT_EVENT_DELETE,
    INPUT_EVENT_HOME,
    INPUT_EVENT_END,
    INPUT_EVENT_PAGE_UP,
    INPUT_EVENT_PAGE_DOWN,
    INPUT_EVENT_TAB
} InputEventType;

/*
 * One recognized key press.
 *
 * `character` holds the Unicode scalar only for INPUT_EVENT_CHARACTER; for
 * every other event type it is unused (0).
 */
typedef struct
{
    InputEventType type;
    uint32_t character;
} InputEvent;

/*
 * Where the parser is inside an escape sequence.
 *
 * Many keys arrive as several bytes (e.g. Up arrow = ESC '[' 'A'). The
 * parser is a state machine that consumes those bytes one call at a time:
 *
 *   NORMAL        - not inside a sequence; single bytes map directly to
 *                   events.
 *   ESCAPE        - saw ESC; waiting to learn whether it is a lone Escape
 *                   key or the start of a sequence.
 *   CSI           - saw ESC '['; waiting for the final byte that names the
 *                   key (A, B, C, D, or a '~'-terminated sequence).
 *   CSI_PARAMETER - inside the numeric parameter of a sequence such as
 *                   "3~" (Delete).
 */
typedef enum
{
    INPUT_PARSER_NORMAL = 0,
    INPUT_PARSER_ESCAPE,
    INPUT_PARSER_CSI,
    INPUT_PARSER_CSI_PARAMETER
} InputParserState;

/*
 * The Input Parser's working state, kept between input_parser_feed() calls.
 *
 *   - state:           where in an escape sequence the parser currently is.
 *   - utf8_code_point: the partially decoded Unicode scalar.
 *   - utf8_minimum:    the smallest value the partially decoded scalar may
 *                      take; used to reject overlong encodings.
 *   - utf8_remaining:  how many continuation bytes are still expected.
 *   - csi_parameter:   the numeric parameter of the current CSI sequence.
 */
typedef struct
{
    InputParserState state;
    uint32_t utf8_code_point;
    uint32_t utf8_minimum;
    unsigned int utf8_remaining;
    unsigned int csi_parameter;
} InputParser;

/*
 * Reset a parser to the NORMAL state so it is ready for a new key sequence.
 *
 * Parameters:
 *   parser - the parser to initialize; may be NULL.
 *
 * Returns:
 *   void.
 */
void input_parser_init(InputParser *parser);

/*
 * Feed one raw byte into the parser.
 *
 * The byte is accumulated into the current key sequence; `event` receives a
 * new event only when the byte completes one. For bytes that merely continue
 * a sequence, `event` is set to INPUT_EVENT_NONE.
 *
 * Parameters:
 *   parser - the parser to feed; must not be NULL.
 *   byte   - the next byte read from the terminal.
 *   event  - out-parameter: receives the recognized key event (or
 *            INPUT_EVENT_NONE); must not be NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments.
 */
bool input_parser_feed(InputParser *parser, unsigned char byte, InputEvent *event);

/*
 * Abort any half-finished sequence and reset the parser to NORMAL.
 *
 * Called when no further bytes arrived in time. If the parser was waiting
 * for the rest of an escape sequence that began with a lone ESC, the timeout
 * turns that ESC into an INPUT_EVENT_ESCAPE; otherwise the event is
 * INPUT_EVENT_NONE.
 *
 * Parameters:
 *   parser - the parser to reset; must not be NULL.
 *   event  - out-parameter: receives the synthesized event; must not be
 *            NULL.
 *
 * Returns:
 *   true on success, false on NULL arguments.
 */
bool input_parser_timeout(InputParser *parser, InputEvent *event);

/*
 * Report whether the parser is in the middle of an escape sequence.
 *
 * The event loop uses this to decide whether the next byte must be read
 * with a timeout (see read_escape_byte() in main.c).
 *
 * Parameters:
 *   parser - the parser to inspect; may be NULL.
 *
 * Returns:
 *   true if `parser` is not NULL and its state is not NORMAL.
 */
bool input_parser_is_waiting(const InputParser *parser);

#endif
