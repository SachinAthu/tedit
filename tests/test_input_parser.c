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
    event = feed(&parser, '\t');
    assert(event.type == INPUT_EVENT_TAB);
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

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '3');
    assert(event.type == INPUT_EVENT_NONE && input_parser_is_waiting(&parser));
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_DELETE && !input_parser_is_waiting(&parser));
}


static void test_home_end_and_page_sequences(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, 'H');
    assert(event.type == INPUT_EVENT_HOME);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, 'F');
    assert(event.type == INPUT_EVENT_END);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '1');
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_HOME);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '4');
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_END);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '5');
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_PAGE_UP);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '6');
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_PAGE_DOWN);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '7');
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_HOME);

    event = feed(&parser, 0x1B);
    event = feed(&parser, '[');
    event = feed(&parser, '8');
    event = feed(&parser, '~');
    assert(event.type == INPUT_EVENT_END);
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

static void test_utf8_scalar_boundaries_and_recovery(void)
{
    InputParser parser;
    InputEvent event;

    input_parser_init(&parser);

    /* Two-, three-, and four-byte sequences each emit one scalar event. */
    event = feed(&parser, 0xC3);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0xA9);
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 0x00E9);

    event = feed(&parser, 0xE2);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x82);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0xAC);
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 0x20AC);

    event = feed(&parser, 0xF0);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x9F);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x98);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x80);
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 0x1F600);

    /* Invalid leading bytes, overlong forms, surrogates, and out-of-range
     * values must never become character events. */
    event = feed(&parser, 0xC0);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0xAF);
    assert(event.type == INPUT_EVENT_NONE);

    event = feed(&parser, 0xE0);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x80);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x80);
    assert(event.type == INPUT_EVENT_NONE);

    event = feed(&parser, 0xED);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0xA0);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x80);
    assert(event.type == INPUT_EVENT_NONE);

    event = feed(&parser, 0xF4);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x90);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x80);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 0x80);
    assert(event.type == INPUT_EVENT_NONE);

    /* A non-continuation byte aborts the incomplete sequence and is parsed
     * again as a new input byte instead of being swallowed. */
    event = feed(&parser, 0xE2);
    assert(event.type == INPUT_EVENT_NONE);
    event = feed(&parser, 'r');
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 'r');
    assert(parser.utf8_remaining == 0);

    /* Timeout also clears an incomplete UTF-8 sequence. */
    event = feed(&parser, 0xF0);
    assert(event.type == INPUT_EVENT_NONE);
    assert(input_parser_timeout(&parser, &event));
    assert(event.type == INPUT_EVENT_NONE && parser.utf8_remaining == 0);
    event = feed(&parser, 's');
    assert(event.type == INPUT_EVENT_CHARACTER && event.character == 's');
}

int main(void)
{
    test_ascii_and_control_keys();
    test_arrow_sequences();
    test_home_end_and_page_sequences();
    test_escape_timeout_and_recovery();
    test_utf8_character();
    test_invalid_utf8_recovers();
    test_utf8_scalar_boundaries_and_recovery();
    return 0;
}
