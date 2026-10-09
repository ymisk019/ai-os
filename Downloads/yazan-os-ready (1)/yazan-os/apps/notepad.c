/* Yazan OS 3.0 - Text Editor / Notepad Application */
#include "notepad.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"
#include "../kernel/pit.h"

#define NOTEPAD_MAX_CHARS 2048

typedef struct {
    char text[NOTEPAD_MAX_CHARS];
    int  cursor_pos;
    int  char_count;
} notepad_state_t;

static void notepad_paint(window_t *win, int cx, int cy, int cw, int ch) {
    notepad_state_t *st = (notepad_state_t *)win->user_data;
    gfx_fill_rect(cx, cy, cw, ch, 0x0D1117);

    /* Text render area */
    int x = cx + 12;
    int y = cy + 12;
    int line_h = 12;

    for (int i = 0; i < st->char_count; i++) {
        char c = st->text[i];
        if (c == '\n') {
            x = cx + 12;
            y += line_h;
            continue;
        }
        if (x + 8 > cx + cw - 12) {
            x = cx + 12;
            y += line_h;
        }
        gfx_char(x, y, c, 1, COL_TEXT_LIGHT, 0x0D1117);
        x += 8;
    }

    /* Blinking Cursor */
    if ((pit_uptime_ms() / 400) % 2 == 0) {
        gfx_fill_rect(x, y, 7, 9, 0x38BDF8);
    }

    /* Bottom status line */
    gfx_fill_rect(cx, cy + ch - 20, cw, 20, 0x161B22);
    char sbuf[64];
    ksnprintf(sbuf, sizeof sbuf, "Characters: %d / %d | UTF-8", st->char_count, NOTEPAD_MAX_CHARS);
    gfx_text(cx + 10, cy + ch - 15, sbuf, 1, COL_TEXT_MUTED);
}

static void notepad_key(window_t *win, char key) {
    notepad_state_t *st = (notepad_state_t *)win->user_data;

    if (key == '\b') {
        if (st->char_count > 0) {
            st->char_count--;
            st->text[st->char_count] = '\0';
        }
    } else if (key == '\n' || (key >= ' ' && key <= '~')) {
        if (st->char_count < NOTEPAD_MAX_CHARS - 1) {
            st->text[st->char_count++] = key;
            st->text[st->char_count] = '\0';
        }
    }
}

window_t *notepad_app_open(void) {
    window_t *win = wm_create_window("Notepad - notes.txt", 160, 120, 420, 290, ICON_NOTEPAD);
    if (!win) return NULL;

    notepad_state_t *st = (notepad_state_t *)kmalloc(sizeof(notepad_state_t));
    if (!st) return NULL;
    memset(st, 0, sizeof(notepad_state_t));

    const char *initial = "Welcome to Yazan OS Notepad!\nYou can type your thoughts and code here.\n\nEnjoy the smooth hybrid UI!";
    strcpy(st->text, initial);
    st->char_count = (int)strlen(initial);

    win->user_data = st;
    win->on_paint = notepad_paint;
    win->on_key = notepad_key;
    return win;
}
