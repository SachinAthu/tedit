#ifndef INPUT_INPUT_PARSER_H
#define INPUT_INPUT_PARSER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_CHARACTER,
    INPUT_EVENT_CTRL_Q,
    INPUT_EVENT_CTRL_S,
    INPUT_EVENT_BACKSPACE,
    INPUT_EVENT_ENTER,
    INPUT_EVENT_ESCAPE,
    INPUT_EVENT_ARROW_LEFT,
    INPUT_EVENT_ARROW_RIGHT,
    INPUT_EVENT_ARROW_UP,
    INPUT_EVENT_ARROW_DOWN
} InputEventType;

typedef struct
{
    InputEventType type;
    uint32_t character;
} InputEvent;

typedef enum
{
    INPUT_PARSER_NORMAL = 0,
    INPUT_PARSER_ESCAPE,
    INPUT_PARSER_CSI
} InputParserState;

typedef struct
{
    InputParserState state;
    uint32_t utf8_code_point;
    uint32_t utf8_minimum;
    unsigned int utf8_remaining;
} InputParser;

void input_parser_init(InputParser *parser);
bool input_parser_feed(InputParser *parser, unsigned char byte, InputEvent *event);
bool input_parser_timeout(InputParser *parser, InputEvent *event);
bool input_parser_is_waiting(const InputParser *parser);

#endif