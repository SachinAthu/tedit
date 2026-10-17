#include "document/document.h"
#include "cursor/cursor.h"
#include "state/editor_state.h"
#include "viewport/viewport.h"
#include "renderer/renderer.h"
#include "terminal/terminal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Unit tests for the core editing behaviour.
 *
 * Each test function below exercises one small piece of behaviour (inserting
 * characters, moving the cursor, scrolling the viewport, drawing, or terminal
 * control) and uses assert() so any failure stops the program with a message.
 * main() at the bottom simply calls every test in order.
 */

/* Compare a line's code points against a NUL-terminated ASCII string. */
static bool line_equals(const DocumentLine *line, const char *text)
{
    size_t length = strlen(text);

    if (line == NULL || line->length != length)
    {
        return false;
    }

    for (size_t index = 0; index < length; ++index)
    {
        if (line->data[index] != (uint32_t)(unsigned char)text[index])
        {
            return false;
        }
    }

    return true;
}

static void test_document_insert_and_delete(void)
{
    Document document;
    DocumentLine *line;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'c'));
    assert(document_insert_character(&document, 0, 1, 'b'));

    line = document_get_line(&document, 0);
    assert(line != NULL);
    assert(line_equals(line, "abc"));

    assert(document_delete_character(&document, 0, 1));
    assert(line_equals(line, "ac"));

    assert(!document_delete_character(&document, 0, 2));
    assert(!document_delete_character(&document, 1, 0));
    assert(line_equals(line, "ac"));

    document_destroy(&document);
}

/* Moving left/right stops at the start and end of the line. */
static void test_cursor_horizontal_movement(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'x'));
    assert(document_insert_character(&document, 0, 1, 'y'));
    cursor_init(&cursor);

    assert(!cursor_move_left(&cursor));
    assert(cursor.column == 0);
    assert(cursor_move_right(&cursor, &document));
    assert(cursor.column == 1);
    assert(cursor_move_left(&cursor));
    assert(cursor.column == 0);
    assert(cursor_move_right(&cursor, &document));
    assert(cursor_move_right(&cursor, &document));
    assert(cursor.column == 2);
    assert(!cursor_move_right(&cursor, &document));
    assert(cursor.column == 2);

    document_destroy(&document);
}

/* Home and End move within logical code-point columns on the current line. */
static void test_cursor_home_and_end(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 0x20AC));
    assert(document_insert_character(&document, 0, 2, 'z'));
    assert(document_split_line(&document, 0, 3));

    cursor_init(&cursor);
    assert(cursor_set_position(&cursor, &document, 0, 2));
    assert(cursor_move_home(&cursor));
    assert(cursor.row == 0 && cursor.column == 0 && cursor.preferred_column == 0);
    assert(!cursor_move_home(&cursor));

    assert(cursor_move_end(&cursor, &document));
    assert(cursor.row == 0 && cursor.column == 3 && cursor.preferred_column == 3);
    assert(!cursor_move_end(&cursor, &document));

    assert(cursor_set_position(&cursor, &document, 1, 0));
    assert(!cursor_move_end(&cursor, &document));
    assert(!cursor_move_end(NULL, &document));
    assert(!cursor_move_end(&cursor, NULL));
    assert(!cursor_move_home(NULL));

    document_destroy(&document);
}

/* Moving up/down keeps the preferred column across lines of uneven length. */
static void test_cursor_vertical_movement_preserves_preferred_column(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_insert_character(&document, 0, 2, 'c'));
    assert(document_insert_character(&document, 0, 3, 'd'));
    assert(document_insert_character(&document, 0, 4, 'e'));
    assert(document_insert_character(&document, 0, 5, 'f'));
    assert(document_split_line(&document, 0, 6));
    assert(document_insert_character(&document, 1, 0, 'x'));
    assert(document_insert_character(&document, 1, 1, 'y'));
    assert(document_split_line(&document, 1, 2));
    assert(document_split_line(&document, 2, 0));
    assert(document_insert_character(&document, 3, 0, '1'));
    assert(document_insert_character(&document, 3, 1, '2'));
    assert(document_insert_character(&document, 3, 2, '3'));
    assert(document_insert_character(&document, 3, 3, '4'));
    assert(document_insert_character(&document, 3, 4, '5'));
    assert(document_insert_character(&document, 3, 5, '6'));

    cursor_init(&cursor);
    assert(cursor_set_position(&cursor, &document, 0, 5));
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.row == 1 && cursor.column == 2);
    assert(cursor.preferred_column == 5);
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.row == 2 && cursor.column == 0);
    assert(cursor.preferred_column == 5);
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.row == 3 && cursor.column == 5);
    assert(cursor.preferred_column == 5);
    assert(cursor_move_up(&cursor, &document));
    assert(cursor.row == 2 && cursor.column == 0);
    assert(cursor_move_up(&cursor, &document));
    assert(cursor.row == 1 && cursor.column == 2);
    assert(cursor_move_up(&cursor, &document));
    assert(cursor.row == 0 && cursor.column == 5);
    assert(!cursor_move_up(&cursor, &document));
    assert(!cursor_move_down(NULL, &document));

    document_destroy(&document);
}

/* Horizontal movement resets the remembered (preferred) column. */
static void test_horizontal_movement_resets_preferred_column(void)
{
    Document document;
    Cursor cursor;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_split_line(&document, 0, 2));
    assert(document_insert_character(&document, 1, 0, 'x'));

    cursor_init(&cursor);
    assert(cursor_set_position(&cursor, &document, 0, 2));
    assert(cursor_move_down(&cursor, &document));
    assert(cursor.column == 1 && cursor.preferred_column == 2);
    assert(cursor_move_left(&cursor));
    assert(cursor.column == 0 && cursor.preferred_column == 0);

    document_destroy(&document);
}

/* A freshly created state starts clean (not dirty). */
static void test_dirty_flag_initial_state(void)
{
    EditorState state;

    assert(editor_state_init(&state));
    assert(!state.is_dirty);
    assert(state.message[0] == '\0');
    assert(editor_state_set_message(&state, "ready"));
    assert(strcmp(state.message, "ready") == 0);
    assert(!editor_state_set_message(NULL, "invalid"));
    assert(!editor_state_set_message(&state, NULL));
    state.is_dirty = true;
    assert(state.is_dirty);
    editor_state_destroy(&state);
}

/* Splitting a line at a column produces a prefix and a suffix line. */
static void test_document_split_line(void)
{
    Document document;
    DocumentLine *first_line;
    DocumentLine *second_line;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_insert_character(&document, 0, 2, 'c'));

    assert(document_split_line(&document, 0, 1));
    assert(document.line_count == 2);
    first_line = document_get_line(&document, 0);
    second_line = document_get_line(&document, 1);
    assert(first_line != NULL && second_line != NULL);
    assert(line_equals(first_line, "a"));
    assert(line_equals(second_line, "bc"));

    assert(document_split_line(&document, 1, 0));
    assert(document.line_count == 3);
    assert(document_get_line(&document, 1)->length == 0);
    assert(document_get_line(&document, 2)->length == 2);

    assert(document_split_line(&document, 2, 2));
    assert(document.line_count == 4);
    assert(document_get_line(&document, 2)->length == 2);
    assert(document_get_line(&document, 3)->length == 0);
    assert(!document_split_line(&document, 4, 0));
    assert(!document_split_line(&document, 0, 2));

    document_destroy(&document);
}

/* Merging two lines joins the second onto the end of the first. */
static void test_document_merge_lines(void)
{
    Document document;
    DocumentLine *line;

    assert(document_init(&document));
    assert(document_insert_character(&document, 0, 0, 'a'));
    assert(document_insert_character(&document, 0, 1, 'b'));
    assert(document_split_line(&document, 0, 2));
    assert(document_insert_character(&document, 1, 0, 'c'));
    assert(document_insert_character(&document, 1, 1, 'd'));

    assert(document_merge_lines(&document, 0));
    assert(document.line_count == 1);
    line = document_get_line(&document, 0);
    assert(line != NULL && line->length == 4);
    assert(line_equals(line, "abcd"));
    assert(!document_merge_lines(&document, 0));
    assert(!document_merge_lines(&document, 1));

    document_destroy(&document);
}

/* Splitting and merging while editing keeps the cursor on the right line. */
static void test_line_editing_cursor_transitions(void)
{
    EditorState state;
    const DocumentLine *previous_line;

    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'a'));
    assert(document_insert_character(&state.document, 0, 1, 'b'));
    assert(cursor_set_position(&state.cursor, &state.document, 0, 1));

    assert(document_split_line(&state.document, state.cursor.row, state.cursor.column));
    assert(cursor_set_position(&state.cursor, &state.document, 1, 0));
    state.is_dirty = true;
    assert(state.cursor.row == 1 && state.cursor.column == 0);
    assert(state.cursor.preferred_column == 0);
    assert(state.document.line_count == 2);
    assert(viewport_ensure_cursor_visible(&state.viewport,
                                          state.cursor.row,
                                          state.cursor.column,
                                          1, 2));
    assert(state.viewport.viewport_row == 1);

    assert(document_insert_character(&state.document, 1, 0, 'c'));
    assert(cursor_set_position(&state.cursor, &state.document, 1, 0));
    previous_line = document_get_line(&state.document, 0);
    assert(previous_line != NULL);
    assert(cursor_set_position(&state.cursor, &state.document,
                               0, previous_line->length));
    assert(document_merge_lines(&state.document, state.cursor.row));
    assert(cursor_set_position(&state.cursor, &state.document, 0, 1));
    assert(state.cursor.row == 0 && state.cursor.column == 1);
    assert(state.cursor.preferred_column == 1);
    assert(state.document.line_count == 1);
    assert(viewport_ensure_cursor_visible(&state.viewport,
                                          state.cursor.row,
                                          state.cursor.column,
                                          1, 2));
    assert(state.viewport.viewport_row == 0);
    assert(state.is_dirty);

    editor_state_destroy(&state);
}

/* The viewport scrolls to keep the cursor visible. */
static void test_viewport_tracks_cursor_visibility(void)
{
    Viewport viewport;

    viewport_init(&viewport);
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 0);

    assert(viewport_ensure_cursor_visible(&viewport, 2, 3, 4, 5));
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 0);

    assert(viewport_ensure_cursor_visible(&viewport, 4, 6, 4, 5));
    assert(viewport.viewport_row == 1);
    assert(viewport.viewport_column == 2);

    assert(viewport_ensure_cursor_visible(&viewport, 0, 1, 4, 5));
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 1);
}

/* Repeated scrolling and screen edges keep the cursor inside the viewport. */
static void test_viewport_repeated_scrolling_and_screen_boundaries(void)
{
    Viewport viewport;
    const size_t screen_rows = 3;
    const size_t screen_columns = 4;
    size_t row;
    size_t column;

    viewport_init(&viewport);

    /* Every cursor position remains inside the viewport after adjustment. */
    for (row = 0; row < 10; ++row)
    {
        for (column = 0; column < 12; ++column)
        {
            assert(viewport_ensure_cursor_visible(
                &viewport, row, column, screen_rows, screen_columns));
            assert(row >= viewport.viewport_row);
            assert(row - viewport.viewport_row < screen_rows);
            assert(column >= viewport.viewport_column);
            assert(column - viewport.viewport_column < screen_columns);
        }
    }

    /* Moving back toward the document origin scrolls both axes back. */
    assert(viewport_ensure_cursor_visible(&viewport, 0, 0,
                                          screen_rows, screen_columns));
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 0);

    /* The last visible cell does not force an unnecessary scroll. */
    assert(viewport_ensure_cursor_visible(&viewport, 2, 3,
                                          screen_rows, screen_columns));
    assert(viewport.viewport_row == 0);
    assert(viewport.viewport_column == 0);

    /* One position beyond the bottom-right visible cell scrolls by one. */
    assert(viewport_ensure_cursor_visible(&viewport, 3, 4,
                                          screen_rows, screen_columns));
    assert(viewport.viewport_row == 1);
    assert(viewport.viewport_column == 1);
}

/* The renderer uses the viewport's row/column offsets when drawing. */
static void test_renderer_uses_viewport_offsets(void)
{
    EditorState state;
    Renderer renderer;
    int output_pipe[2];
    int saved_stdout;
    char output[2048];
    ssize_t output_length;
    const ScreenCell *cell;

    assert(editor_state_init(&state));
    assert(document_insert_character(&state.document, 0, 0, 'a'));
    assert(document_insert_character(&state.document, 0, 1, 'b'));
    assert(document_insert_character(&state.document, 0, 2, 'c'));
    assert(document_split_line(&state.document, 0, 3));
    assert(document_insert_character(&state.document, 1, 0, 'd'));
    assert(document_insert_character(&state.document, 1, 1, 'e'));
    assert(document_insert_character(&state.document, 1, 2, 'f'));
    assert(document_insert_character(&state.document, 1, 3, 'g'));
    assert(document_insert_character(&state.document, 1, 4, 'h'));
    assert(cursor_set_position(&state.cursor, &state.document, 1, 3));
    assert(viewport_ensure_cursor_visible(&state.viewport, 1, 3, 2, 3));
    assert(state.viewport.viewport_row == 0);
    assert(state.viewport.viewport_column == 1);

    /* Force a vertical offset too, as if the document had been scrolled. */
    state.viewport.viewport_row = 1;
    state.viewport.viewport_column = 2;

    assert(renderer_init(&renderer, 4, 3));
    assert(pipe(output_pipe) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(output_pipe[1], STDOUT_FILENO) == STDOUT_FILENO);
    close(output_pipe[1]);

    assert(renderer_render(&renderer, &state));

    assert(dup2(saved_stdout, STDOUT_FILENO) == STDOUT_FILENO);
    close(saved_stdout);
    output_length = read(output_pipe[0], output, sizeof(output) - 1);
    assert(output_length >= 0);
    output[output_length] = '\0';
    close(output_pipe[0]);

    cell = screen_buffer_get(&renderer.screen_buffer, 0, 0);
    assert(cell != NULL && cell->code_point == 'f');
    cell = screen_buffer_get(&renderer.screen_buffer, 0, 1);
    assert(cell != NULL && cell->code_point == 'g');
    cell = screen_buffer_get(&renderer.screen_buffer, 0, 2);
    assert(cell != NULL && cell->code_point == 'h');
    assert(strstr(output, "\x1b[1;2H") != NULL);

    renderer_destroy(&renderer);
    editor_state_destroy(&state);
}

/* The final row is the Status Bar and reports name, dirty state, and position. */
static void test_renderer_draws_status_bar(void)
{
    EditorState state;
    Renderer renderer;
    const ScreenCell *cell;
    size_t status_row;

    assert(editor_state_init(&state));
    assert(renderer_init(&renderer, 5, 40));
    state.file_path = malloc(sizeof("notes.txt"));
    assert(state.file_path != NULL);
    memcpy(state.file_path, "notes.txt", sizeof("notes.txt"));
    state.is_dirty = true;
    state.cursor.row = 1;
    state.cursor.column = 4;

    /* Capture the renderer output so the test does not write control bytes. */
    int output_pipe[2];
    int saved_stdout;
    assert(pipe(output_pipe) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(output_pipe[1], STDOUT_FILENO) == STDOUT_FILENO);
    close(output_pipe[1]);

    assert(renderer_render(&renderer, &state));
    assert(dup2(saved_stdout, STDOUT_FILENO) == STDOUT_FILENO);
    close(saved_stdout);
    close(output_pipe[0]);

    status_row = renderer.screen_buffer.rows - 1;
    cell = screen_buffer_get(&renderer.screen_buffer, status_row, 0);
    assert(cell != NULL && cell->code_point == 'n');
    cell = screen_buffer_get(&renderer.screen_buffer, status_row, 10);
    assert(cell != NULL && cell->code_point == '[');
    cell = screen_buffer_get(&renderer.screen_buffer, status_row, 11);
    assert(cell != NULL && cell->code_point == '+');

    /* Cursor information is right-aligned and uses one-based coordinates. */
    const char expected_right[] = "Ln 2, Col 5";
    size_t right_start = renderer.screen_buffer.columns - (sizeof(expected_right) - 1);
    for (size_t index = 0; index < sizeof(expected_right) - 1; ++index)
    {
        cell = screen_buffer_get(&renderer.screen_buffer, status_row, right_start + index);
        assert(cell != NULL && cell->code_point == (uint32_t)(unsigned char)expected_right[index]);
    }

    renderer_destroy(&renderer);
    editor_state_destroy(&state);
}

/* The final row displays the current Message Bar text and clears stale text. */
static void test_renderer_draws_message_bar(void)
{
    EditorState state;
    Renderer renderer;
    const ScreenCell *cell;
    char output[2048];
    ssize_t output_length;

    assert(editor_state_init(&state));
    assert(renderer_init(&renderer, 4, 24));
    assert(editor_state_set_message(&state, "Saved successfully."));

    /* Capture output to verify the Message Bar style without terminal output. */
    int output_pipe[2];
    int saved_stdout;
    assert(pipe(output_pipe) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(output_pipe[1], STDOUT_FILENO) == STDOUT_FILENO);
    close(output_pipe[1]);
    assert(renderer_render(&renderer, &state));
    assert(dup2(saved_stdout, STDOUT_FILENO) == STDOUT_FILENO);
    close(saved_stdout);
    output_length = read(output_pipe[0], output, sizeof(output) - 1);
    assert(output_length >= 0);
    output[output_length] = '\0';
    close(output_pipe[0]);
    assert(strstr(output, "\x1b[30;47m") != NULL);

    cell = screen_buffer_get(&renderer.screen_buffer, renderer.screen_buffer.rows - 2, 0);
    assert(cell != NULL && cell->code_point == 'S');
    cell = screen_buffer_get(&renderer.screen_buffer, renderer.screen_buffer.rows - 2, 5);
    assert(cell != NULL && cell->code_point == ' ');

    /* The row before the Message Bar is blank; the Status Bar is last. */
    for (size_t column = 0; column < renderer.screen_buffer.columns; ++column)
    {
        cell = screen_buffer_get(&renderer.screen_buffer, renderer.document_rows, column);
        assert(cell != NULL && cell->code_point == (uint32_t)' ');
    }
    cell = screen_buffer_get(&renderer.screen_buffer, renderer.screen_buffer.rows - 1, 0);
    assert(cell != NULL && cell->code_point == '[');

    assert(editor_state_set_message(&state, ""));
    assert(pipe(output_pipe) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(output_pipe[1], STDOUT_FILENO) == STDOUT_FILENO);
    close(output_pipe[1]);
    assert(renderer_render(&renderer, &state));
    assert(dup2(saved_stdout, STDOUT_FILENO) == STDOUT_FILENO);
    close(saved_stdout);
    close(output_pipe[0]);

    for (size_t column = 0; column < renderer.screen_buffer.columns; ++column)
    {
        cell = screen_buffer_get(&renderer.screen_buffer, renderer.screen_buffer.rows - 2, column);
        assert(cell != NULL && cell->code_point == (uint32_t)' ');
    }

    renderer_destroy(&renderer);
    editor_state_destroy(&state);
}

/* A zero-line Document becomes editable on the first insertion or Enter. */
static void test_zero_line_document_editing(void)
{
    Document document;
    DocumentLine *line;

    assert(document_init(&document));
    document_destroy(&document);

    assert(document_insert_character(&document, 0, 0, 'q'));
    assert(document.line_count == 1);
    line = document_get_line(&document, 0);
    assert(line != NULL && line->length == 1);
    assert(line->data[0] == (uint32_t)'q');

    document_destroy(&document);
    assert(document_init(&document));
    document_destroy(&document);

    assert(!document_insert_character(&document, 1, 0, 'q'));
    assert(!document_insert_character(&document, 0, 1, 'q'));
    assert(document.line_count == 0);

    assert(document_split_line(&document, 0, 0));
    assert(document.line_count == 2);
    assert(document_get_line(&document, 0) != NULL);
    assert(document_get_line(&document, 1) != NULL);
    assert(document_get_line(&document, 0)->length == 0);
    assert(document_get_line(&document, 1)->length == 0);
    assert(!document_split_line(&document, 2, 0));

    document_destroy(&document);
}

/* The renderer refuses to draw when the cursor is outside the viewport. */
static void test_renderer_rejects_cursor_outside_viewport(void)
{
    EditorState state;
    Renderer renderer;

    assert(editor_state_init(&state));
    assert(renderer_init(&renderer, 4, 4));

    state.viewport.viewport_row = 1;
    assert(!renderer_render(&renderer, &state));

    state.viewport.viewport_row = 0;
    state.viewport.viewport_column = 1;
    assert(!renderer_render(&renderer, &state));

    renderer_destroy(&renderer);
    editor_state_destroy(&state);
}

/* Rendering an empty-file Document produces a blank frame safely. */
static void test_renderer_renders_zero_line_document(void)
{
    EditorState state;
    Renderer renderer;
    int output_pipe[2];
    int saved_stdout;
    char output[512];
    ssize_t output_length;

    assert(editor_state_init(&state));
    document_destroy(&state.document); /* Model an empty file loaded from disk. */
    cursor_init(&state.cursor);
    viewport_init(&state.viewport);

    assert(renderer_init(&renderer, 4, 3));
    assert(pipe(output_pipe) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(output_pipe[1], STDOUT_FILENO) == STDOUT_FILENO);
    close(output_pipe[1]);

    assert(renderer_render(&renderer, &state));

    assert(dup2(saved_stdout, STDOUT_FILENO) == STDOUT_FILENO);
    close(saved_stdout);
    output_length = read(output_pipe[0], output, sizeof(output) - 1);
    assert(output_length >= 0);
    output[output_length] = '\0';
    close(output_pipe[0]);

    for (size_t row = 0; row < renderer.screen_buffer.rows; ++row)
    {
        for (size_t column = 0; column < renderer.screen_buffer.columns; ++column)
        {
            const ScreenCell *cell = screen_buffer_get(
                &renderer.screen_buffer, row, column);
            assert(cell != NULL && cell->code_point == (uint32_t)' ');
        }
    }

    assert(strstr(output, "\x1b[1;1H") != NULL);

    renderer_destroy(&renderer);
    editor_state_destroy(&state);
}

/* Cursor and viewport stay in sync through long vertical and horizontal scrolls. */
static void test_cursor_and_viewport_remain_synchronized_during_scrolling(void)
{
    EditorState state;
    const size_t visible_rows = 8;
    const size_t visible_columns = 10;
    size_t row;
    size_t column;

    assert(editor_state_init(&state));

    /* Create forty logical lines, including the final empty line. */
    for (row = 0; row < 39; ++row)
    {
        const DocumentLine *line = document_get_line(&state.document, row);
        assert(line != NULL);
        assert(document_split_line(&state.document, row, line->length));
    }

    assert(cursor_set_position(&state.cursor, &state.document, 0, 0));
    for (row = 1; row < state.document.line_count; ++row)
    {
        assert(cursor_move_down(&state.cursor, &state.document));
        assert(viewport_ensure_cursor_visible(&state.viewport,
                                              state.cursor.row,
                                              state.cursor.column,
                                              visible_rows,
                                              visible_columns));
        assert(state.cursor.row - state.viewport.viewport_row < visible_rows);
    }

    for (row = state.document.line_count - 1; row > 0; --row)
    {
        assert(cursor_move_up(&state.cursor, &state.document));
        assert(viewport_ensure_cursor_visible(&state.viewport,
                                              state.cursor.row,
                                              state.cursor.column,
                                              visible_rows,
                                              visible_columns));
        assert(state.cursor.row >= state.viewport.viewport_row);
        assert(state.cursor.row - state.viewport.viewport_row < visible_rows);
    }
    assert(state.cursor.row == 0);
    assert(state.viewport.viewport_row == 0);

    /* Exercise horizontal scrolling on one line longer than the viewport. */
    for (column = 0; column < 100; ++column)
    {
        assert(document_insert_character(&state.document, 0, column, 'x'));
    }
    assert(cursor_set_position(&state.cursor, &state.document, 0, 0));

    for (column = 0; column < 100; ++column)
    {
        assert(cursor_move_right(&state.cursor, &state.document));
        assert(viewport_ensure_cursor_visible(&state.viewport,
                                              state.cursor.row,
                                              state.cursor.column,
                                              visible_rows,
                                              visible_columns));
        assert(state.cursor.column >= state.viewport.viewport_column);
        assert(state.cursor.column - state.viewport.viewport_column < visible_columns);
    }

    while (state.cursor.column > 0)
    {
        assert(cursor_move_left(&state.cursor));
        assert(viewport_ensure_cursor_visible(&state.viewport,
                                              state.cursor.row,
                                              state.cursor.column,
                                              visible_rows,
                                              visible_columns));
        assert(state.cursor.column >= state.viewport.viewport_column);
        assert(state.cursor.column - state.viewport.viewport_column < visible_columns);
    }
    assert(state.viewport.viewport_column == 0);

    editor_state_destroy(&state);
}

/* Entering/leaving the alternate screen toggles auto-wrap correctly. */
static void test_terminal_disables_and_restores_autowrap(void)
{
    TerminalState terminal = {0};
    int output_pipe[2];
    int saved_stdout;
    char output[128];
    ssize_t output_length;

    assert(pipe(output_pipe) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(output_pipe[1], STDOUT_FILENO) == STDOUT_FILENO);
    close(output_pipe[1]);

    assert(terminal_enter_alternate_screen(&terminal));
    assert(terminal.autowrap_disabled);
    terminal_leave_alternate_screen(&terminal);
    assert(!terminal.autowrap_disabled);
    assert(!terminal.alternate_screen_enabled);

    assert(dup2(saved_stdout, STDOUT_FILENO) == STDOUT_FILENO);
    close(saved_stdout);
    output_length = read(output_pipe[0], output, sizeof(output) - 1);
    assert(output_length >= 0);
    output[output_length] = '\0';
    close(output_pipe[0]);

    assert(strstr(output, "\x1b[?1049h") != NULL);
    assert(strstr(output, "\x1b[?7l") != NULL);
    assert(strstr(output, "\x1b[?7h") != NULL);
    assert(strstr(output, "\x1b[?1049l") != NULL);
}

/* Invalid (zero) dimensions leave the viewport offsets unchanged. */
static void test_viewport_invalid_dimensions_preserve_offsets(void)
{
    Viewport viewport;

    viewport_init(&viewport);
    assert(viewport_ensure_cursor_visible(&viewport, 8, 9, 3, 4));
    assert(viewport.viewport_row == 6);
    assert(viewport.viewport_column == 6);

    assert(!viewport_ensure_cursor_visible(&viewport, 0, 0, 0, 4));
    assert(!viewport_ensure_cursor_visible(&viewport, 0, 0, 3, 0));
    assert(viewport.viewport_row == 6);
    assert(viewport.viewport_column == 6);
}

/* Zero dimensions and a NULL viewport are rejected. */
static void test_viewport_rejects_invalid_dimensions(void)
{
    Viewport viewport;

    viewport_init(&viewport);
    assert(!viewport_ensure_cursor_visible(&viewport, 0, 0, 0, 5));
    assert(!viewport_ensure_cursor_visible(&viewport, 0, 0, 5, 0));
    assert(!viewport_ensure_cursor_visible(NULL, 0, 0, 5, 5));
}

int main(void)
{
    test_document_insert_and_delete();
    test_cursor_horizontal_movement();
    test_cursor_home_and_end();
    test_cursor_vertical_movement_preserves_preferred_column();
    test_horizontal_movement_resets_preferred_column();
    test_dirty_flag_initial_state();
    test_document_split_line();
    test_document_merge_lines();
    test_zero_line_document_editing();
    test_line_editing_cursor_transitions();
    test_viewport_tracks_cursor_visibility();
    test_viewport_repeated_scrolling_and_screen_boundaries();
    test_renderer_uses_viewport_offsets();
    test_renderer_draws_status_bar();
    test_renderer_draws_message_bar();
    test_renderer_rejects_cursor_outside_viewport();
    test_renderer_renders_zero_line_document();
    test_cursor_and_viewport_remain_synchronized_during_scrolling();
    test_terminal_disables_and_restores_autowrap();
    test_viewport_invalid_dimensions_preserve_offsets();
    test_viewport_rejects_invalid_dimensions();

    return 0;
}