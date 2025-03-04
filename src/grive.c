#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>

#include "la.h"
#include "common.h"
#include "editor.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WWIDTH 1440 
#define WHEIGHT 900

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH  (FONT_WIDTH  / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)

size_t FONT_SCALE = 5.0f;

#define COLOR_WHITE (Uint32)0xffffffff
#define COLOR_CREME (Uint32)0xf2ebebff

// #define THIN_CURSOR
#ifdef THIN_CURSOR
#define CURSOR_WIDTH 5
#endif 

#define UNHEX(color) \
    (color >> (8 * 3)) & 0xFF, \
    (color >> (8 * 2)) & 0xFF, \
    (color >> (8 * 1)) & 0xFF, \
    (color >> (8 * 0)) & 0xFF  \


#define CURSOR_COLOR UNHEX(0xf2ebebff)

#define ASCII_DISPLAY_LOW  32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
    SDL_Texture *spritesheet;
    SDL_Rect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

SDL_Surface *surface_from_file(const char *file_path)
{
    int width, height, n;
    unsigned char *pixels = stbi_load(file_path, &width, &height, &n, STBI_rgb_alpha);
    if (pixels == NULL) {
        fprintf(stderr, "ERROR: could not load file %s: %s\n",
                file_path, stbi_failure_reason());
        exit(1);
    }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const Uint32 rmask = 0xff000000;
    const Uint32 gmask = 0x00ff0000;
    const Uint32 bmask = 0x0000ff00;
    const Uint32 amask = 0x000000ff;
#else // little endian, like x86
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif

    const int depth = 32;
    const int pitch = 4*width;

    return scp(SDL_CreateRGBSurfaceFrom(
                   (void*)pixels, width, height, depth, pitch,
                   rmask, gmask, bmask, amask));
}


Font font_load_from_file(SDL_Renderer *renderer, const char *file_path)
{
    Font font = {0};

    SDL_Surface *font_surface = surface_from_file(file_path);
    scc(SDL_SetColorKey(font_surface, SDL_TRUE, 0xFF000000));
    font.spritesheet = scp(SDL_CreateTextureFromSurface(renderer, font_surface));
    SDL_FreeSurface(font_surface);

    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii) {
        const size_t index = ascii - ASCII_DISPLAY_LOW;
        const size_t col = index % FONT_COLS;
        const size_t row = index / FONT_COLS;
        font.glyph_table[index] = (SDL_Rect) {
            .x = (int) col * FONT_CHAR_WIDTH,
            .y = (int) row * FONT_CHAR_HEIGHT,
            .w = FONT_CHAR_WIDTH,
            .h = FONT_CHAR_HEIGHT,
        };
    }

    return font;
}

void render_char(SDL_Renderer *renderer, const Font *font, char c, Vec2 pos, float scale)
{
    const SDL_Rect dst = {
        .x = (int) floorf(pos.x),
        .y = (int) floorf(pos.y),
        .w = (int) floorf(FONT_CHAR_WIDTH * scale),
        .h = (int) floorf(FONT_CHAR_HEIGHT * scale),
    };

    size_t index = '?' - ASCII_DISPLAY_LOW;
    if (ASCII_DISPLAY_LOW <= c && c <= ASCII_DISPLAY_HIGH) {
        index = c - ASCII_DISPLAY_LOW;
    }

    scc(SDL_RenderCopy(renderer, font->spritesheet, &font->glyph_table[index], &dst));
}

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(
            texture,
            (color >> (8 * 0)) & 0xff,
            (color >> (8 * 1)) & 0xff,
            (color >> (8 * 2)) & 0xff));

    scc(SDL_SetTextureAlphaMod(texture, (color >> (8 * 3)) & 0xff));
}

void render_text_sized(SDL_Renderer *renderer, Font *font, const char *text, size_t text_size, Vec2 pos, Uint32 color, float scale)
{
    set_texture_color(font->spritesheet, color);

    Vec2 p = pos;
    for (size_t i = 0; i < text_size; ++i) {
        render_char(renderer, font, text[i], p, scale);
        p.x += FONT_CHAR_WIDTH * scale;
    }
}

void render_cursor(SDL_Renderer *renderer, const Font *font, Editor *editor)
{
    const Vec2 pos =
        vec2((float) editor->cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
              (float) editor->cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE);

    const SDL_Rect rect = {
        .x = (int) floorf(pos.x),
        .y = (int) floorf(pos.y),
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE,
    };

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    const char *c = editor_char_under_cursor(editor);
    if (c) {
        set_texture_color(font->spritesheet, 0xFF000000);
        render_char(renderer, font, *c, pos, FONT_SCALE);
    }
}

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./grive [FILE-PATH]\n");
}

Editor editor = {0};

int main(int argc, char *argv[])
{
    const char *file_path = NULL;

    if (argc > 1) {
        file_path = argv[1];
    }

    if (file_path) {
        editor_load_from_file(&editor, file_path);
    }

    scc(SDL_Init(SDL_INIT_VIDEO));

    Uint32 windowFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE;

    // Window 
    SDL_Window *window = scp(SDL_CreateWindow("Grive", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        WWIDTH, 
        WHEIGHT, 
        windowFlags
    ));

    // Renderer
    SDL_Renderer *renderer =
        scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    // Load font
    Font font = font_load_from_file(renderer, "font/charmap-oldschool_white.png");

    bool quit = false;
    while (!quit) {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = true;
            }
            break;

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_BACKSPACE: {
                    editor_backspace(&editor);
                    editor_remove_line(&editor);
                }
                break;

                case SDLK_F2: {
                    if (file_path) {
                        editor_save_to_file(&editor, file_path);
                    }
                }
                break;

                case SDLK_RETURN: {
                    editor_insert_new_line(&editor);
                }
                break;

                case SDLK_ESCAPE: {
                    editor_delete(&editor);
                }
                break;

                case SDLK_TAB: {
                    editor_tab_space(&editor);
                }
                break;

                case SDLK_UP: {
                    editor_move_cursor_up(&editor);
                }
                break;

                case SDLK_DOWN: {
                    editor_move_cursor_down(&editor);
                }
                break;

                case SDLK_LEFT: {
                    editor_move_cursor_left(&editor);
                }
                break;

                case SDLK_RIGHT: {
                    editor_move_cursor_right(&editor);
                }
                break;
                }
            }
            break;

            case SDL_TEXTINPUT: {
                editor_insert_text_before_cursor(&editor, event.text.text);
            }
            break;
            }
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        for (size_t row = 0; row < editor.len; ++row) {
            const Line *line = editor.lines + row;
            render_text_sized(renderer, 
                &font, 
                line->chars, 
                line->len, 
                (Vec2){0.0f, (float) row * FONT_CHAR_HEIGHT * FONT_SCALE}, 
                0xFFFFFFFF, 
                FONT_SCALE);
        }
        render_cursor(renderer, &font, &editor);

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}

#define SV_IMPLEMENTATION
#include "./sv.h"
