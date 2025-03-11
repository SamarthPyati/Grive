#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPE
#include <SDL_opengl.h>
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

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


#define FPS 30
#define DELTA_TIME (1.0f / FPS)

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


// Camera 
#define CAM_BUFFER vec2s(200, 0)

typedef struct {
    Vec2 pos;
    Vec2 vel;
} Camera;

Camera camera = {0};


Vec2 window_size(SDL_Window *window) {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return vec2s((float)w, (float)h);
}

Vec2 camera_project_point(SDL_Window *window, Vec2 point) {
    // Add half of window dimension to properly project on screen 
    return vec2_add(vec2_add(vec2_sub(point, camera.pos), vec2_mul(window_size(window), vec2c(0.5))), CAM_BUFFER);
}

void render_cursor(SDL_Renderer *renderer, const Font *font, Editor *editor, Camera *camera, SDL_Window *window)
{
    Vec2 pos =
        vec2_sub(vec2s((float) editor->cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
              (float) editor->cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE), camera->pos);
    pos = camera_project_point(window, pos);

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

// Main editor struct 
Editor editor = {0};

// Courtesy: https://github.com/tsoding/opengl-template
void MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
    (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
    type, severity, message);
}

#define OPENGL_RENDERER
#if defined(OPENGL_RENDERER)
// OPEN GL RENDERER
int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    scc(SDL_Init(SDL_INIT_VIDEO));

    Uint32 windowFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

    // Window 
    SDL_Window *window = scp(SDL_CreateWindow("Grive", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        WWIDTH, 
        WHEIGHT, 
        windowFlags
    ));

    {   // Set attributes and gl profile 
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        int debugFlag = SDL_GL_CONTEXT_DEBUG_FLAG;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &debugFlag);

        int major, minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        fprintf(stdout, "[INFO] GL Version: %d.%d\n", major, minor);    

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    }

    void *glContext = scp(SDL_GL_CreateContext(window));

    // Get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string

    printf("[INFO] Renderer: %s\n", renderer);
    printf("[INFO] OpenGL Version Supported %s\n", version);

    if (glewInit() != GLEW_OK) {
        RAISE("GL", "Could not initialize GLEW!"); exit(1);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(&MessageCallback, 0);
    } else {
        fprintf(stderr, "[WARNING] GL extension GLEW_ARB_debug_output is not available.\n");
    }

    return 0;
}

#else 
// STANDARD SDL RENDERER 
int main(int argc, char *argv[])
{
    const char *file_path = NULL;

    if (argc > 1) {
        file_path = argv[1];
    }

    if (file_path) {
        FILE *f = fopen(file_path, "r");
        if (f != NULL) {
            editor_load_from_file(&editor, f);
        } 
        fclose(f);
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

    const Uint32 start_time = SDL_GetTicks();

    // Main Loop 
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


        // Scrolling
        {
            Vec2 cursor_pos =
                        vec2s((float) editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
                              (float) editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE);   
            Vec2 velocity = vec2_sub(cursor_pos, camera.pos);       // direction or vel
            // lower down the velocity by 50 %
            velocity = vec2_mul(velocity, vec2c(0.1));
            camera.pos = vec2_add(camera.pos, vec2_mul(velocity, vec2c(DELTA_TIME)));
        }   

        for (size_t row = 0; row < editor.len; ++row) {
            const Line *line = editor.lines + row;
            Vec2 line_pos = vec2_sub(vec2s(0.0f, (float) row * FONT_CHAR_HEIGHT * FONT_SCALE), camera.pos);
            line_pos = camera_project_point(window, line_pos);

            render_text_sized(renderer, 
                &font, 
                line->chars, 
                line->len, 
                line_pos, 
                0xFFFFFFFF, 
                FONT_SCALE);
        }
        render_cursor(renderer, &font, &editor, &camera, window);

        SDL_RenderPresent(renderer);

        // Set SDL_Delay
        const Uint32 duration = SDL_GetTicks() - start_time;
        const Uint32 delta_time_ms = 1000 / FPS;
        if (duration < delta_time_ms) {
            SDL_Delay(duration - delta_time_ms);
        }
    }

    SDL_Quit();

    return 0;
}
#endif

#define SV_IMPLEMENTATION
#include "./sv.h"