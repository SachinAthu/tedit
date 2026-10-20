#include "input/input_parser.h"

#include <stddef.h>

/*
 * This file turns raw terminal bytes into InputEvents.
 *
 * A parser is a small state machine because many keystrokes arrive as
 * multi-byte sequences: an Up arrow is ESC '[' 'A', Delete is ESC '[' '3'
 * '~', and non-ASCII characters arrive as UTF-8 byte runs. The parser
 * remembers where it is inside such a sequence between input_parser_feed()
 * calls and emits an event only when a complete key has been recognized.
 */

#define INPUT_CONTROL_Q 0x11
#define INPUT_CONTROL_S 0x13
#define INPUT_BACKSPACE 0x7F
#define INPUT_BACKSPACE_ALT 0x08
#define INPUT_ESCAPE 0x1B
#define INPUT_ENTER 0x0D
#define INPUT_TAB 0x09
#define INPUT_LINE_FEED 0x0A
#define UTF8_CONTINUATION_MASK 0xC0
#define UTF8_CONTINUATION_PREFIX 0x80
#define UTF8_TWO_BYTE_PREFIX 0xC0
#define UTF8_THREE_BYTE_PREFIX 0xE0
#define UTF8_FOUR_BYTE_PREFIX 0xF0

/*
 * Reset a parser to the NORMAL state so it is ready for a new key sequence.
 *
 * Parameters:
 *   parser - the parser to initialize; may be NULL.
 *
 * Returns:
 *   void.
 */
void input_parser_init(InputParser *parser)
{
    if (parser == NULL)
    {
        return;
    }

    parser->state = INPUT_PARSER_NORMAL;
    parser->utf8_code_point = 0;
    parser->utf8_minimum = 0;
    parser->utf8_remaining = 0;
    parser->csi_parameter = 0;
}

/*
 * Mark an event as "no key recognized yet".
 *
 * Parameters:
 *   event - the event to clear; must not be NULL.
 *
 * Returns:
 *   void.
 */
static void input_event_clear(InputEvent *event)
{
    event->type = INPUT_EVENT_NONE;
    event->character = 0;
}

/*
 * Handle one byte of a UTF-8 character being assembled.
 *
 * When a continuation byte completes the code point, the accumulated scalar
 * is validated (range, surrogate exclusion, and the `utf8_minimum` check
 * that rejects overlong encodings) and emitted as an INPUT_EVENT_CHARACTER.
 *
 * Parameters:
 *   parser - the parser holding the partial UTF-8 state.
 *   byte   - the next raw byte.
 *   event  - out-parameter: receives the completed character event when the
 *            code point finishes.
 *
 * Returns:
 *   always true (all input is consumed by this handler).
 */
static bool input_parser_feed_utf8(InputParser *parser, unsigned char byte, InputEvent *event)
{
    if (parser->utf8_remaining > 0)
    {
        if ((byte & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PREFIX)
        {
            parser->utf8_remaining = 0;
            parser->utf8_code_point = 0;
            parser->utf8_minimum = 0;
            return input_parser_feed(parser, byte, event);
        }

        parser->utf8_code_point = (parser->utf8_code_point << 6) | (uint32_t)(byte & 0x3F);
        parser->utf8_remaining--;

        if (parser->utf8_remaining == 0)
        {
            const uint32_t code_point = parser->utf8_code_point;
            const bool valid = code_point >= parser->utf8_minimum &&
                               code_point <= 0x10FFFF &&
                               !(code_point >= 0xD800 && code_point <= 0xDFFF);
            parser->utf8_code_point = 0;
            parser->utf8_minimum = 0;

            if (valid)
            {
                event->type = INPUT_EVENT_CHARACTER;
                event->character = code_point;
            }
        }

        return true;
    }

    if (byte >= 0xC2 && byte <= 0xDF)
    {
        parser->utf8_code_point = (uint32_t)(byte & 0x1F);
        parser->utf8_minimum = 0x80;
        parser->utf8_remaining = 1;
    }
    else if (byte >= 0xE0 && byte <= 0xEF)
    {
        parser->utf8_code_point = (uint32_t)(byte & 0x0F);
        parser->utf8_minimum = 0x800;
        parser->utf8_remaining = 2;
    }
    else if (byte >= 0xF0 && byte <= 0xF4)
    {
        parser->utf8_code_point = (uint32_t)(byte & 0x07);
        parser->utf8_minimum = 0x10000;
        parser->utf8_remaining = 3;
    }

    return true;
}

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
bool input_parser_feed(InputParser *parser, unsigned char byte, InputEvent *event)
{
    if (parser == NULL || event == NULL)
    {
        return false;
    }

    input_event_clear(event);

    if (parser->utf8_remaining > 0)
    {
        return input_parser_feed_utf8(parser, byte, event);
    }

    if (parser->state == INPUT_PARSER_NORMAL)
    {
        if (byte == INPUT_ESCAPE)
        {
            parser->state = INPUT_PARSER_ESCAPE;
        }
        else if (byte == INPUT_CONTROL_Q)
        {
            event->type = INPUT_EVENT_CTRL_Q;
        }
        else if (byte == INPUT_CONTROL_S)
        {
            event->type = INPUT_EVENT_CTRL_S;
        }
        else if (byte == INPUT_BACKSPACE || byte == INPUT_BACKSPACE_ALT)
        {
            event->type = INPUT_EVENT_BACKSPACE;
        }
        else if (byte == INPUT_ENTER || byte == INPUT_LINE_FEED)
        {
            event->type = INPUT_EVENT_ENTER;
        }
        else if (byte == INPUT_TAB)
        {
            event->type = INPUT_EVENT_TAB;
        }
        else if (byte >= 0x20 && byte <= 0x7E)
        {
            event->type = INPUT_EVENT_CHARACTER;
            event->character = byte;
        }
        else if (byte >= 0x80)
        {
            return input_parser_feed_utf8(parser, byte, event);
        }

        return true;
    }

    if (parser->state == INPUT_PARSER_ESCAPE)
    {
        if (byte == '[')
        {
            parser->state = INPUT_PARSER_CSI;
            parser->csi_parameter = 0;
        }
        else if (byte == INPUT_ESCAPE)
        {
            parser->state = INPUT_PARSER_ESCAPE;
        }
        else
        {
            parser->state = INPUT_PARSER_NORMAL;
        }

        return true;
    }

    if (parser->state == INPUT_PARSER_CSI_PARAMETER)
    {
        if (byte >= '0' && byte <= '9')
        {
            if (parser->csi_parameter <= 999)
            {
                parser->csi_parameter = parser->csi_parameter * 10U + (unsigned int)(byte - '0');
            }
            return true;
        }

        if (byte == '~')
        {
            if (parser->csi_parameter == 1 || parser->csi_parameter == 7)
            {
                event->type = INPUT_EVENT_HOME;
            }
            else if (parser->csi_parameter == 4 || parser->csi_parameter == 8)
            {
                event->type = INPUT_EVENT_END;
            }
            else if (parser->csi_parameter == 3)
            {
                event->type = INPUT_EVENT_DELETE;
            }
            else if (parser->csi_parameter == 5)
            {
                event->type = INPUT_EVENT_PAGE_UP;
            }
            else if (parser->csi_parameter == 6)
            {
                event->type = INPUT_EVENT_PAGE_DOWN;
            }
        }
        parser->state = INPUT_PARSER_NORMAL;
        parser->csi_parameter = 0;

        /* A control byte can be a real key even if it interrupts a malformed
         * CSI parameter. Reprocess it after abandoning the incomplete sequence
         * so Ctrl-S, Ctrl-Q, Escape, and similar keys are not swallowed. */
        if (byte < 0x20 || byte == 0x7F)
        {
            return input_parser_feed(parser, byte, event);
        }

        return true;
    }

    if (byte >= '0' && byte <= '9')
    {
        parser->state = INPUT_PARSER_CSI_PARAMETER;
        parser->csi_parameter = (unsigned int)(byte - '0');
        return true;
    }

    parser->state = INPUT_PARSER_NORMAL;
    parser->csi_parameter = 0;

    /* Do not consume a standalone control key merely because it interrupted
     * an escape sequence. Start parsing that byte again from NORMAL state. */
    if (byte < 0x20 || byte == 0x7F)
    {
        return input_parser_feed(parser, byte, event);
    }

    if (byte == 'D')
    {
        event->type = INPUT_EVENT_ARROW_LEFT;
    }
    else if (byte == 'H')
    {
        event->type = INPUT_EVENT_HOME;
    }
    else if (byte == 'F')
    {
        event->type = INPUT_EVENT_END;
    }
    else if (byte == 'C')
    {
        event->type = INPUT_EVENT_ARROW_RIGHT;
    }
    else if (byte == 'A')
    {
        event->type = INPUT_EVENT_ARROW_UP;
    }
    else if (byte == 'B')
    {
        event->type = INPUT_EVENT_ARROW_DOWN;
    }

    return true;
}

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
bool input_parser_timeout(InputParser *parser, InputEvent *event)
{
    if (parser == NULL || event == NULL)
    {
        return false;
    }

    const InputParserState previous_state = parser->state;

    parser->state = INPUT_PARSER_NORMAL;
    parser->utf8_code_point = 0;
    parser->utf8_minimum = 0;
    parser->utf8_remaining = 0;
    parser->csi_parameter = 0;
    input_event_clear(event);
    if (previous_state == INPUT_PARSER_ESCAPE)
    {
        event->type = INPUT_EVENT_ESCAPE;
    }
    return true;
}

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
bool input_parser_is_waiting(const InputParser *parser)
{
    return parser != NULL && parser->state != INPUT_PARSER_NORMAL;
}
