#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_WINDOWS 16
#define TITLEBAR_HEIGHT 28

struct window;
typedef void (*win_paint_fn)(struct window *win, int cx, int cy, int cw, int ch);
typedef void (*win_mouse_fn)(struct window *win, int rx, int ry, int btns, int clicked);
typedef void (*win_key_fn)(struct window *win, char key);

typedef struct window {
    int          id;
    char         title[64];
    int          x, y, w, h;
    int          orig_x, orig_y, orig_w, orig_h;
    int          icon_type;
    int          is_active;
    int          is_minimized;
    int          is_maximized;
    int          is_closed;

    win_paint_fn on_paint;
    win_mouse_fn on_mouse;
    win_key_fn   on_key;
    void        *user_data;
} window_t;

void      wm_init(int screen_w, int screen_h);
window_t *wm_create_window(const char *title, int x, int y, int w, int h, int icon);
void      wm_close_window(window_t *win);
void      wm_focus_window(window_t *win);
void      wm_toggle_minimize(window_t *win);
void      wm_toggle_maximize(window_t *win);
window_t *wm_get_active_window(void);
window_t *wm_get_window(int idx);
int       wm_get_window_count(void);

void      wm_handle_mouse(int mx, int my, int btns, int clicked);
void      wm_handle_key(char key);
void      wm_render(void);
