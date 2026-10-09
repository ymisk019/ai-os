/* Yazan OS 3.0 - Interactive Graphical Terminal Application
 * Features command parsing, color output, scroll history, and system utilities. */
#include "terminal.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"
#include "../kernel/pmm.h"
#include "../kernel/pit.h"
#include "../kernel/panic.h"
#include "../kernel/io.h"

#define TERM_MAX_LINES 32
#define TERM_LINE_LEN  80

typedef struct {
    char lines[TERM_MAX_LINES][TERM_LINE_LEN];
    rgb_t line_colors[TERM_MAX_LINES];
    int  num_lines;
    char input_buf[TERM_LINE_LEN];
    int  input_len;
} term_state_t;

static void term_add_line(term_state_t *st, const char *text, rgb_t color) {
    if (st->num_lines >= TERM_MAX_LINES) {
        for (int i = 0; i < TERM_MAX_LINES - 1; i++) {
            memcpy(st->lines[i], st->lines[i + 1], TERM_LINE_LEN);
            st->line_colors[i] = st->line_colors[i + 1];
        }
        st->num_lines = TERM_MAX_LINES - 1;
    }
    strncpy(st->lines[st->num_lines], text, TERM_LINE_LEN - 1);
    st->lines[st->num_lines][TERM_LINE_LEN - 1] = '\0';
    st->line_colors[st->num_lines] = color;
    st->num_lines++;
}

static void term_execute_command(term_state_t *st, const char *cmd) {
    char buf[128];
    ksnprintf(buf, sizeof buf, "yazan@os:~$ %s", cmd);
    term_add_line(st, buf, COL_TEXT_LIGHT);

    if (strcmp(cmd, "help") == 0) {
        term_add_line(st, "Available commands:", COL_PRIMARY);
        term_add_line(st, "  help     - Show this help message", COL_TEXT_MUTED);
        term_add_line(st, "  info     - Display OS info & ASCII logo", COL_TEXT_MUTED);
        term_add_line(st, "  mem      - Show physical and heap memory usage", COL_TEXT_MUTED);
        term_add_line(st, "  clear    - Clear the terminal screen", COL_TEXT_MUTED);
        term_add_line(st, "  date     - Show system uptime", COL_TEXT_MUTED);
        term_add_line(st, "  apps     - List running applications", COL_TEXT_MUTED);
        term_add_line(st, "  panic    - Trigger Red Screen of Death (Test)", COL_ERROR);
        term_add_line(st, "  reboot   - Reboot the computer", COL_WARN);
    } else if (strcmp(cmd, "info") == 0 || strcmp(cmd, "about") == 0 || strcmp(cmd, "neofetch") == 0) {
        term_add_line(st, "  __  __                  ____   _____ ", 0x38BDF8);
        term_add_line(st, " \\ \\/ /___ _____ ____   / __ \\ / ___/ ", 0x38BDF8);
        term_add_line(st, "  \\  // __ `/ _ `/ _ \\ / / / / \\__ \\  ", 0x38BDF8);
        term_add_line(st, "  / // /_/ / // / // // /_/ / ___/ /  ", 0x38BDF8);
        term_add_line(st, " /_/ \\__,_/\\_,_/\\_,_/ \\____/ /____/   ", 0x38BDF8);
        term_add_line(st, "OS       : Yazan OS 3.0 Fire Edition", COL_OK);
        term_add_line(st, "Kernel   : x86_64 Long Mode Hybrid", COL_TEXT_LIGHT);
        term_add_line(st, "UI Style : Windows + macOS + Android", COL_PRIMARY);
        term_add_line(st, "Author   : Developed with passion for Yazan", COL_TEXT_YELLOW);
    } else if (strcmp(cmd, "mem") == 0) {
        char mbuf[96];
        uint64_t total = pmm_get_total_ram() >> 20;
        uint64_t usable = pmm_get_usable_ram() >> 20;
        uint64_t used = pmm_get_used_ram() >> 20;
        uint64_t free_mb = pmm_get_free_ram() >> 20;
        ksnprintf(mbuf, sizeof mbuf, "RAM Total : %lu MB | Usable: %lu MB", (unsigned long)total, (unsigned long)usable);
        term_add_line(st, mbuf, COL_TEXT_LIGHT);
        ksnprintf(mbuf, sizeof mbuf, "RAM Used  : %lu MB | Free: %lu MB", (unsigned long)used, (unsigned long)free_mb);
        term_add_line(st, mbuf, COL_OK);
        ksnprintf(mbuf, sizeof mbuf, "Heap Used : %lu KB / %lu KB",
                  (unsigned long)(heap_get_used() >> 10), (unsigned long)(heap_get_total() >> 10));
        term_add_line(st, mbuf, COL_TEXT_MUTED);
    } else if (strcmp(cmd, "clear") == 0) {
        st->num_lines = 0;
    } else if (strcmp(cmd, "date") == 0) {
        char tbuf[64];
        uint64_t up = pit_uptime_ms() / 1000;
        ksnprintf(tbuf, sizeof tbuf, "Uptime: %lu seconds (%lu minutes)", (unsigned long)up, (unsigned long)(up / 60));
        term_add_line(st, tbuf, COL_TEXT_LIGHT);
    } else if (strcmp(cmd, "apps") == 0) {
        term_add_line(st, "Installed Applications:", COL_PRIMARY);
        term_add_line(st, "  1. Terminal (Active)", COL_OK);
        term_add_line(st, "  2. Calculator", COL_TEXT_LIGHT);
        term_add_line(st, "  3. File Explorer", COL_TEXT_LIGHT);
        term_add_line(st, "  4. Notepad (Text Editor)", COL_TEXT_LIGHT);
        term_add_line(st, "  5. System Settings / About", COL_TEXT_LIGHT);
        term_add_line(st, "  6. Task Manager", COL_TEXT_LIGHT);
    } else if (strcmp(cmd, "panic") == 0) {
        kpanic("User triggered test panic via Terminal 'panic' command");
    } else if (strcmp(cmd, "reboot") == 0) {
        term_add_line(st, "Rebooting computer...", COL_WARN);
        outb(0x64, 0xFE); /* PS/2 reboot */
        cpu_halt_forever();
    } else if (strlen(cmd) > 0) {
        char err[96];
        ksnprintf(err, sizeof err, "bash: command not found: %s", cmd);
        term_add_line(st, err, COL_ERROR);
        term_add_line(st, "Type 'help' for a list of commands.", COL_TEXT_DIM);
    }
}

static void term_paint(window_t *win, int cx, int cy, int cw, int ch) {
    term_state_t *st = (term_state_t *)win->user_data;
    gfx_fill_rect(cx, cy, cw, ch, 0x0A0E1A);

    int scale = 1;
    int line_h = 10;
    int max_visible = ch / line_h - 2;
    int start_line = 0;
    if (st->num_lines > max_visible) {
        start_line = st->num_lines - max_visible;
    }

    int y = cy + 6;
    for (int i = start_line; i < st->num_lines; i++) {
        gfx_text(cx + 8, y, st->lines[i], scale, st->line_colors[i]);
        y += line_h;
    }

    /* Prompt & current input line */
    gfx_text(cx + 8, y, "yazan@os:~$ ", scale, 0x38BDF8);
    gfx_text(cx + 8 + 88, y, st->input_buf, scale, COL_TEXT_LIGHT);

    /* Blinking cursor effect */
    if ((pit_uptime_ms() / 400) % 2 == 0) {
        int cur_x = cx + 8 + 88 + (int)strlen(st->input_buf) * 8;
        gfx_fill_rect(cur_x, y, 7, 9, 0x38BDF8);
    }
}

static void term_key(window_t *win, char key) {
    term_state_t *st = (term_state_t *)win->user_data;

    if (key == '\n') {
        st->input_buf[st->input_len] = '\0';
        term_execute_command(st, st->input_buf);
        st->input_len = 0;
        st->input_buf[0] = '\0';
    } else if (key == '\b') {
        if (st->input_len > 0) {
            st->input_len--;
            st->input_buf[st->input_len] = '\0';
        }
    } else if (key >= ' ' && key <= '~') {
        if (st->input_len < TERM_LINE_LEN - 2) {
            st->input_buf[st->input_len++] = key;
            st->input_buf[st->input_len] = '\0';
        }
    }
}

window_t *terminal_app_open(void) {
    window_t *win = wm_create_window("Terminal - yazan@os:~", 60, 60, 520, 320, ICON_TERMINAL);
    if (!win) return NULL;

    term_state_t *st = (term_state_t *)kmalloc(sizeof(term_state_t));
    if (!st) return NULL;
    memset(st, 0, sizeof(term_state_t));

    term_add_line(st, "Welcome to Yazan OS 3.0 Terminal Shell!", 0x38BDF8);
    term_add_line(st, "Type 'help' to view available system commands.", 0x94A3B8);
    term_add_line(st, "Type 'info' or 'mem' for system diagnostics.", 0x94A3B8);
    term_add_line(st, "", COL_TEXT_LIGHT);

    win->user_data = st;
    win->on_paint = term_paint;
    win->on_key = term_key;
    return win;
}
