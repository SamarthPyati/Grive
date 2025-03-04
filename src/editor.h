#ifndef EDITOR_H_
#define EDITOR_H_

#include <stdlib.h>

typedef struct {
    size_t cap;
    size_t len;
    char *chars;
} Line;

void line_append_text(Line *line, const char *text);
void line_append_text_sized(Line *line, const char *text, size_t text_size);
void line_insert_text_before(Line *line, const char *text, size_t *col);
void line_insert_text_sized_before(Line *line, const char *text, size_t text_size, size_t *col);
void line_backspace(Line *line, size_t *col);
void line_delete(Line *line, size_t *col);

typedef struct {
    size_t cap;
    size_t len;
    Line *lines;
    size_t cursor_row;
    size_t cursor_col;
} Editor;

// Editor file I/O operations
void editor_save_to_file(const Editor *editor, const char *file_path);
void editor_load_from_file(Editor *editor, const char *file_path);

// Editor cursor navigation
void editor_move_cursor_left(Editor *editor);
void editor_move_cursor_right(Editor *editor);
void editor_move_cursor_up(Editor *editor);
void editor_move_cursor_down(Editor *editor);

// Editor operations
void editor_insert_text_before_cursor(Editor *editor, const char *text);
void editor_insert_new_line(Editor *editor);
void editor_backspace(Editor *editor);
void editor_tab_space(Editor *editor);
void editor_delete(Editor *editor);
void editor_remove_line(Editor *editor);
const char *editor_char_under_cursor(const Editor *editor);

#endif // EDITOR_H_
