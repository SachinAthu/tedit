#include "input/input_parser.h"

#include <assert.h>
#include <stddef.h>

static InputEvent feed(InputParser *parser, unsigned char byte)
{
    InputEvent event;
    assert(input_parser_feed(parser, byte, &event));
    return event;
}

static void test_ascii_and_control_keys(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);
    event = feed(&parser, 'x');
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 'x');
    event = feed(&parser, 0x11);
    assert(event.type == INPUT_EVENT_CTRL_Q);
    event = feed(&parser, 0x13);
    assert(event.type == INPUT_EVENT_CTRL_S);
    event = feed(&parser, 0x7F);
    assert(event.type == INPUT_EVENT_BACKSPACE);
    event = feed(&parser, 0x08);
    assert(event.type == INPUT_EVENT_BACKSPACE);
    event = feed(&parser, '\r');
    assert(event.type == INPUT_EVENT_ENTER);
    event = feed(&parser, '\n');
    assert(event.type == INPUT_EVENT_ENTER);
}

static void test_arrow_sequences(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);
    event = feed(&parser, 0x1B);
    assert(event.type == INPUT_EVENT_NONE);
    assert(input_parser_is_waiting(&parser));
    event = feed(&parser, '[');
    assert(event.type == INPUT_EVENT_NONE);
    assert(input_parser_is_waiting(&parser));
    event = feed(&parser, 'A');
    assert(event.type == INPUT_EVENT_ARROW_UP);
    assert(!input_parser_is_waiting(&parser));

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, 'B');
    assert(event.type == INPUT_EVENT_ARROW_DOWN);
    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, 'C');
    assert(event.type == INPUT_EVENT_ARROW_RIGHT);
    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, 'D');
    assert(event.type == INPUT_EVENT_ARROW_LEFT);
}

static void test_escape_timeout_and_recovery(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);
    event = feed(&parser, 0x1B);
    assert(event.type == INPUT_EVENT_NONE);
    assert(input_parser_timeout(&parser, &event));
    assert(event.type == INPUT_EVENT_ESCAPE);
    assert(!input_parser_is_waiting(&parser));

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    assert(input_parser_timeout(&parser, &event));
    assert(event.type == INPUT_EVENT_NONE);

    event = feed(&parser, 'z');
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 'z');

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, 'X');
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 'q');
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 'q');
}

static void test_utf8_character(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);
    event = feed(&parser, 0xE2);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x82);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0xAC);
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 0x20AC);
}

static void test_invalid_utf8_recovers(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);
    event = feed(&parser, 0xE2);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 'a');
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 'a');
}

int main(void)
{
    test_ascii_and_control_keys();
    test_arrow_sequences();
    test_escape_timeout_and_recovery();
    test_utf8_character();
    test_invalid_utf8_recovers();
    return 0;
}