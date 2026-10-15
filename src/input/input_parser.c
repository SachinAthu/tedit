#include "input/input_parser.h"

#include <stddef.h>

#define INPUT_CONTROL_Q 0x11
#define INPUT_CONTROL_S 0x13
#define INPUT_BACKSPACE 0x7F
#define INPUT_BACKSPACE_ALT 0x08
#define INPUT_ESCAPE 0x1B
#define INPUT_ENTER 0x0D
#define INPUT_LINE_FEED 0x0A
#define UTF8_CONTINUATION_MASK 0xC0
#define UTF8_CONTINUATION_PREFIX 0x80
#define UTF8_TWO_BYTE_PREFIX 0xC0
#define UTF8_THREE_BYTE_PREFIX 0xE0
#define UTF8_FOUR_BYTE_PREFIX 0xF0

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
}

static void input_event_clear(InputEvent *event)
{
    event->type = INPUT_EVENT_NONE;
    event->character = 0;
}

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

    parser->state = INPUT_PARSER_NORMAL;

    if (byte == 'D')
    {
        event->type = INPUT_EVENT_ARROW_LEFT;
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
    input_event_clear(event);
    if (previous_state == INPUT_PARSER_ESCAPE)
    {
        event->type = INPUT_EVENT_ESCAPE;
    }
    return true;
}

bool input_parser_is_waiting(const InputParser *parser)
{
    return parser != NULL && parser->state != INPUT_PARSER_NORMAL;
}