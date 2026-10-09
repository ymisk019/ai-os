/* Yazan OS 3.0 - Brick Rush: a 2D brick-breaker arcade game.
 * Controls: A / D or mouse to move the paddle, SPACE or click to launch,
 * R to restart. Physics use fixed-point maths (no floats: the kernel is
 * built without SSE). */
#include "brickbreak.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"
#include "../kernel/pit.h"

#define PF_X        10          /* playfield offset inside window content */
#define PF_Y        34
#define PF_W        360
#define PF_H        340
#define BRICK_ROWS  6
#define BRICK_COLS  9
#define BRICK_W     38
#define BRICK_H     14
#define BRICK_GAP   2
#define BRICK_TOP   40
#define PADDLE_W    72
#define PADDLE_H    10
#define PADDLE_Y    (PF_H - 24)
#define BALL_SIZE   6
#define SUBPX       16          /* fixed-point units per pixel */
#define STEP_MS     16          /* one physics step = one 16 ms tick */
#define MAX_STEPS   4
#define START_LIVES 3
#define PADDLE_KEY_STEP 14

typedef enum { BB_READY, BB_PLAYING, BB_OVER, BB_WIN } bb_phase_t;

typedef struct {
    uint8_t    alive[BRICK_ROWS][BRICK_COLS];
    int        paddle_x;
    int        ball_x, ball_y;     /* SUBPX units */
    int        ball_dx, ball_dy;   /* SUBPX units per step */
    int        score;
    int        lives;
    int        bricks_left;
    bb_phase_t phase;
    uint64_t   last_ms;
} bb_game_t;

static const rgb_t bb_row_col[BRICK_ROWS] = {
    0xEF4444, 0xF97316, 0xFACC15, 0x22C55E, 0x38BDF8, 0x8B5CF6
};

static int bb_abs(int v) { return v < 0 ? -v : v; }
static int bb_min(int a, int b) { return a < b ? a : b; }
static int bb_max(int a, int b) { return a > b ? a : b; }

static void bb_reset_ball(bb_game_t *g) {
    g->ball_x = (g->paddle_x + PADDLE_W / 2 - BALL_SIZE / 2) * SUBPX;
    g->ball_y = (PADDLE_Y - BALL_SIZE - 1) * SUBPX;
    g->ball_dx = 0;
    g->ball_dy = 0;
}

static void bb_new_game(bb_game_t *g) {
    memset(g, 0, sizeof(*g));
    for (int r = 0; r < BRICK_ROWS; r++)
        for (int c = 0; c < BRICK_COLS; c++)
            g->alive[r][c] = 1;
    g->bricks_left = BRICK_ROWS * BRICK_COLS;
    g->paddle_x = (PF_W - PADDLE_W) / 2;
    g->lives = START_LIVES;
    g->phase = BB_READY;
    bb_reset_ball(g);
}

static void bb_launch(bb_game_t *g) {
    if (g->phase != BB_READY) return;
    g->ball_dx = (g->score % 20 == 0) ? 36 : -36;
    g->ball_dy = -48;
    g->phase = BB_PLAYING;
}

static void bb_move_paddle(bb_game_t *g, int delta) {
    g->paddle_x = bb_max(0, bb_min(PF_W - PADDLE_W, g->paddle_x + delta));
}

static void bb_lose_life(bb_game_t *g) {
    g->lives--;
    if (g->lives <= 0) {
        g->phase = BB_OVER;
    } else {
        g->phase = BB_READY;
        bb_reset_ball(g);
    }
}

static void bb_step(bb_game_t *g) {
    if (g->phase != BB_PLAYING) {
        if (g->phase == BB_READY) bb_reset_ball(g);
        return;
    }

    g->ball_x += g->ball_dx;
    g->ball_y += g->ball_dy;

    /* Side and top walls */
    int max_x = (PF_W - BALL_SIZE) * SUBPX;
    if (g->ball_x < 0)            { g->ball_x = 0;     g->ball_dx = -g->ball_dx; }
    else if (g->ball_x > max_x)   { g->ball_x = max_x; g->ball_dx = -g->ball_dx; }
    if (g->ball_y < 0)            { g->ball_y = 0;     g->ball_dy = -g->ball_dy; }

    int x = g->ball_x / SUBPX;
    int y = g->ball_y / SUBPX;

    /* Ball fell below the paddle */
    if (y > PF_H) {
        bb_lose_life(g);
        return;
    }

    /* Paddle collision: bounce upward, angle depends on hit position */
    if (g->ball_dy > 0 &&
        y + BALL_SIZE >= PADDLE_Y && y + BALL_SIZE <= PADDLE_Y + PADDLE_H + 6 &&
        x + BALL_SIZE >= g->paddle_x && x <= g->paddle_x + PADDLE_W) {
        int off = (x + BALL_SIZE / 2) - (g->paddle_x + PADDLE_W / 2);
        int dx = off * 3;
        g->ball_dx = bb_max(-64, bb_min(64, dx));
        g->ball_dy = -bb_abs(g->ball_dy);
        g->ball_y = (PADDLE_Y - BALL_SIZE) * SUBPX;
        y = PADDLE_Y - BALL_SIZE;
    }

    /* Brick collision: first hit only this step */
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (!g->alive[r][c]) continue;
            int bx = c * (BRICK_W + BRICK_GAP) + 1;
            int by = BRICK_TOP + r * (BRICK_H + BRICK_GAP);
            if (x < bx + BRICK_W && x + BALL_SIZE > bx &&
                y < by + BRICK_H && y + BALL_SIZE > by) {
                g->alive[r][c] = 0;
                g->bricks_left--;
                g->score += 10;

                int ov_x = bb_min(x + BALL_SIZE, bx + BRICK_W) - bb_max(x, bx);
                int ov_y = bb_min(y + BALL_SIZE, by + BRICK_H) - bb_max(y, by);
                if (ov_x < ov_y) g->ball_dx = -g->ball_dx;
                else             g->ball_dy = -g->ball_dy;

                r = BRICK_ROWS;   /* leave both loops */
                break;
            }
        }
    }

    if (g->bricks_left <= 0) g->phase = BB_WIN;
}

static void bb_paint(window_t *win, int cx, int cy, int cw, int ch) {
    bb_game_t *g = (bb_game_t *)win->user_data;

    /* Advance physics by elapsed time, so speed does not depend on frame rate */
    uint64_t now = pit_uptime_ms();
    if (g->last_ms == 0 || now < g->last_ms) g->last_ms = now;
    int steps = (int)((now - g->last_ms) / STEP_MS);
    if (steps > MAX_STEPS) {
        steps = MAX_STEPS;
        g->last_ms = now;             /* drop the backlog instead of lagging */
    } else {
        g->last_ms += (uint64_t)steps * STEP_MS;
    }
    for (int i = 0; i < steps; i++) bb_step(g);

    /* Background and HUD */
    gfx_fill_rect(cx, cy, cw, ch, 0x0B1220);

    char buf[48];
    ksnprintf(buf, sizeof buf, "Score: %d", g->score);
    gfx_text(cx + 10, cy + 8, buf, 1, COL_TEXT_LIGHT);
    ksnprintf(buf, sizeof buf, "Lives: %d", g->lives);
    gfx_text(cx + cw - 80, cy + 8, buf, 1, COL_WARN);

    /* Playfield */
    int ox = cx + PF_X, oy = cy + PF_Y;
    gfx_fill_rrect(ox - 2, oy - 2, PF_W + 4, PF_H + 4, 6, 0x1E293B);
    gfx_fill_rect(ox, oy, PF_W, PF_H, 0x0F172A);

    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (!g->alive[r][c]) continue;
            int bx = ox + c * (BRICK_W + BRICK_GAP) + 1;
            int by = oy + BRICK_TOP + r * (BRICK_H + BRICK_GAP);
            gfx_fill_rrect(bx, by, BRICK_W, BRICK_H, 3, bb_row_col[r]);
        }
    }

    gfx_fill_rrect(ox + g->paddle_x, oy + PADDLE_Y, PADDLE_W, PADDLE_H, 4, COL_PRIMARY);
    gfx_fill_rrect(ox + g->ball_x / SUBPX, oy + g->ball_y / SUBPX,
                   BALL_SIZE, BALL_SIZE, 3, COL_WHITE);

    /* Status messages */
    int mid_x = ox + PF_W / 2;
    int mid_y = oy + PF_H / 2 + 20;
    if (g->phase == BB_READY)
        gfx_text_center(mid_x, mid_y, "SPACE or click to launch", 1, COL_TEXT_MUTED);
    else if (g->phase == BB_OVER)
        gfx_text_center(mid_x, mid_y, "GAME OVER - press R", 1, COL_ERROR);
    else if (g->phase == BB_WIN)
        gfx_text_center(mid_x, mid_y, "YOU WIN! - press R", 1, COL_OK);

    gfx_text(cx + 10, oy + PF_H + 8, "A/D or mouse: move   SPACE: launch   R: restart",
             1, COL_TEXT_DIM);
}

static void bb_mouse(window_t *win, int rx, int ry, int btns, int clicked) {
    (void)ry;
    (void)btns;
    bb_game_t *g = (bb_game_t *)win->user_data;

    g->paddle_x = bb_max(0, bb_min(PF_W - PADDLE_W, (rx - PF_X) - PADDLE_W / 2));

    if (clicked) {
        if (g->phase == BB_READY)                       bb_launch(g);
        else if (g->phase == BB_OVER || g->phase == BB_WIN) bb_new_game(g);
    }
}

static void bb_key(window_t *win, char key) {
    bb_game_t *g = (bb_game_t *)win->user_data;

    switch (key) {
    case 'a': case 'A': bb_move_paddle(g, -PADDLE_KEY_STEP); break;
    case 'd': case 'D': bb_move_paddle(g,  PADDLE_KEY_STEP); break;
    case ' ': case '\n':
        if (g->phase == BB_READY) bb_launch(g);
        else if (g->phase == BB_OVER || g->phase == BB_WIN) bb_new_game(g);
        break;
    case 'r': case 'R': bb_new_game(g); break;
    default: break;
    }
}

window_t *brickbreak_app_open(void) {
    window_t *win = wm_create_window("Brick Rush", 220, 80, 386, 420, ICON_BRICKS);
    if (!win) return NULL;

    bb_game_t *g = (bb_game_t *)kmalloc(sizeof(bb_game_t));
    if (!g) return NULL;
    bb_new_game(g);

    win->user_data = g;
    win->on_paint = bb_paint;
    win->on_mouse = bb_mouse;
    win->on_key = bb_key;
    return win;
}
