#ifndef CURSOR_CURSOR_H
#define CURSOR_CURSOR_H

#include "document/document.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    size_t row;
    size_t column;
} Cursor;

void cursor_init(Cursor *cursor);

bool cursor_set_position(Cursor *cursor, const Document *document, size_t row, size_t column);

bool cursor_advance(Cursor *cursor, const Document *document);

bool cursor_move_left(Cursor *cursor);

bool cursor_move_right(Cursor *cursor, const Document *document);

#endif