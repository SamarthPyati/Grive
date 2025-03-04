#include "editor.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "sv.h"

#define LINE_INIT_CAPACITY 1024
#define EDITOR_INIT_CAPACITY 128

static void editor_create_first_new_line(Editor *editor);

static void line_grow(Line *line, size_t n)
{
    size_t new_capacity = line->cap;

    assert(new_capacity >= line->len);
    while (new_capacity - line->len < n) {
        if (new_capacity == 0) {
            new_capacity = LINE_INIT_CAPACITY;
        } else {
            new_capacity *= 2;
        }
    }

    if (new_capacity != line->cap) {
        line->chars = realloc(line->chars, new_capacity);
        line->cap = new_capacity;
    }
}

void line_append_text(Line *line, const char *text)
{
    line_append_text_sized(line, text, strlen(text));
}

void line_append_text_sized(Line *line, const char *text, size_t text_size)
{
    size_t col = line->len;
    line_insert_text_sized_before(line, text, text_size, &col);
}

void line_insert_text_sized_before(Line *line, const char *text, size_t text_size, size_t *col)
{
    if (*col > line->len) {
        *col = line->len;
    }

    line_grow(line, text_size);

    memmove(line->chars + *col + text_size,
            line->chars + *col,
            line->len - *col);
    memcpy(line->chars + *col, text, text_size);
    line->len += text_size;
    *col += text_size;
}

void line_insert_text_before(Line *line, const char *text, size_t *col)
{
    line_insert_text_sized_before(line, text, strlen(text), col);
}

void line_backspace(Line *line, size_t *col)
{
    if (*col > line->len) {
        *col = line->len;
    }

    if (*col > 0 && line->len > 0) {
        memmove(line->chars + *col - 1,
                line->chars + *col,
                line->len - *col);
        line->len -= 1;
        *col -= 1;
    }
}

void line_delete(Line *line, size_t *col)
{
    if (*col > line->len) {
        *col = line->len;
    }

    if (*col < line->len && line->len > 0) {
        memmove(line->chars + *col,
                line->chars + *col + 1,
                line->len - *col);
        line->len -= 1;
    }
}

static void editor_grow(Editor *editor, size_t n)
{
    size_t new_capacity = editor->cap;

    assert(new_capacity >= editor->len);
    while (new_capacity - editor->len < n) {
        if (new_capacity == 0) {
            new_capacity = EDITOR_INIT_CAPACITY;
        } else {
            new_capacity *= 2;
        }
    }

    if (new_capacity != editor->cap) {
        editor->lines = realloc(editor->lines, new_capacity * sizeof(editor->lines[0]));
        editor->cap = new_capacity;
    }
}

void editor_insert_new_line(Editor *editor)
{
    if (editor->cursor_row > editor->len) {
        editor->cursor_row = editor->len;
    }

    editor_grow(editor, 1);

    const size_t line_size = sizeof(editor->lines[0]);
    memmove(editor->lines + editor->cursor_row + 1,
            editor->lines + editor->cursor_row,
            (editor->len - editor->cursor_row) * line_size);
    memset(&editor->lines[editor->cursor_row + 1], 0, line_size);
    editor->cursor_row += 1;
    editor->cursor_col = 0;
    editor->len += 1;
}

static void editor_create_first_new_line(Editor *editor)
{
    if (editor->cursor_row >= editor->len) {
        if (editor->len > 0) {
            editor->cursor_row = editor->len - 1;
        } else {
            editor_grow(editor, 1);
            memset(&editor->lines[editor->len], 0, sizeof(editor->lines[0]));
            editor->len += 1;
        }
    }
}

void editor_insert_text_before_cursor(Editor *editor, const char *text)
{
    editor_create_first_new_line(editor);
    line_insert_text_before(&editor->lines[editor->cursor_row], text, &editor->cursor_col);
}

void editor_backspace(Editor *editor)
{
    editor_create_first_new_line(editor);
    line_backspace(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

void editor_delete(Editor *editor)
{
    editor_create_first_new_line(editor);
    line_delete(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

void editor_tab_space(Editor *editor) {
    const char *tab_space = "    ";
    line_insert_text_before(&editor->lines[editor->cursor_row], tab_space, &editor->cursor_col);
}

void editor_remove_line(Editor *editor) {
    /* For removing empty line only for now (Problem integrating with backspace) */
    if (editor->cursor_col == 0 && 
        editor->cursor_row > 0 && 
        editor->lines[editor->cursor_row].len == 0) {
            
        size_t line_mem_sz = sizeof(editor->lines[0]);
        memmove(editor->lines + (editor->cursor_row), 
            editor->lines + (editor->cursor_row + 1), 
            (editor->len - editor->cursor_row) * line_mem_sz);

        editor->cursor_row -= 1;
        editor->cursor_col = editor->lines[editor->cursor_row].len;
    }
}

const char *editor_char_under_cursor(const Editor *editor)
{
    if (editor->cursor_row < editor->len) {
        if (editor->cursor_col < editor->lines[editor->cursor_row].len) {
            return &editor->lines[editor->cursor_row].chars[editor->cursor_col];
        }
    }
    return NULL;
}

void editor_save_to_file(const Editor *editor, const char *file_path)
{
    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        fprintf(stdout, "ERROR: could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    for (size_t row = 0; row < editor->len; ++row) {
        fwrite(editor->lines[row].chars, 1, editor->lines[row].len, f);
        fputc('\n', f);
    }

    fclose(f);
}

void editor_load_from_file(Editor *editor, const char *file_path)
{
    assert(editor->lines == NULL && "You can only load files into an empty editor");
    editor_create_first_new_line(editor);

    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        fprintf(stderr, "ERROR: could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    static char chunk[640 * 1024];

    while (!feof(f)) {
        size_t n = fread(chunk, 1, sizeof(chunk), f);

        String_View chunk_sv = {
            .data = chunk,
            .count = n
        };

        while (chunk_sv.count > 0) {
            String_View chunk_line = {0};
            Line *line = &editor->lines[editor->len - 1];
            if (sv_try_chop_by_delim(&chunk_sv, '\n', &chunk_line)) {
                line_append_text_sized(line, chunk_line.data, chunk_line.count);
                editor_insert_new_line(editor);
            } else {
                line_append_text_sized(line, chunk_sv.data, chunk_sv.count);
                chunk_sv = SV_NULL;
            }
        }
    }

    editor->cursor_row = 0;

    fclose(f);
}


void editor_move_cursor_left(Editor *editor) {
    if (editor->cursor_col > 0) {
        editor->cursor_col -= 1;
    }
}

void editor_move_cursor_right(Editor *editor) {
    if (editor->cursor_col < editor->lines[editor->cursor_row].len) {
        editor->cursor_col += 1;
    }
}

void editor_move_cursor_up(Editor *editor) {
    if (editor->cursor_row > 0) {
        // Move cursor row up by one
        editor->cursor_row -= 1;    
        // If lenght of upper line is smaller than cursor_col, snap the cursor to the end of the line on moving up
        if (editor->lines[editor->cursor_row].len < editor->cursor_col) {
            editor->cursor_col = editor->lines[editor->cursor_row].len;
        }
    }
}

void editor_move_cursor_down(Editor *editor) {
    if (editor->cursor_row < editor->len - 1) {
        editor->cursor_row += 1;
        // If lenght of lower line is smaller than cursor_col, snap the cursor to the end of the line on moving down
        if (editor->lines[editor->cursor_row].len < editor->cursor_col) {
            editor->cursor_col = editor->lines[editor->cursor_row].len;
        }
    }
}
