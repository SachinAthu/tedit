#ifndef CURSOR_CURSOR_H
#define CURSOR_CURSOR_H

#include "document/document.h"

#include <stdbool.h>
#include <stddef.h>

/* The cursor's position within the document. All coordinates are zero-based. */
typedef struct
{
    size_t row;              /* Which line the cursor is on. */
    size_t column;           /* Which character position within that line. */
    size_t preferred_column; /* The column to return to when moving up or down. */
} Cursor;

void cursor_init(Cursor *cursor);

bool cursor_set_position(Cursor *cursor, const Document *document, size_t row, size_t column);

bool cursor_advance(Cursor *cursor, const Document *document);

bool cursor_move_left(Cursor *cursor);

bool cursor_move_right(Cursor *cursor, const Document *document);

bool cursor_move_up(Cursor *cursor, const Document *document);

bool cursor_move_down(Cursor *cursor, const Document *document);

#endif