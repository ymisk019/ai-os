#include "mouse.h"
#include "ps2.h"
#include "pic.h"
#include "serial.h"

static int mouse_x = 512;
static int mouse_y = 384;
static int max_w = 1024;
static int max_h = 768;

static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[3];
static int mouse_buttons = 0;
static int prev_buttons = 0;

void mouse_init(int screen_w, int screen_h) {
    max_w = screen_w;
    max_h = screen_h;
    mouse_x = screen_w / 2;
    mouse_y = screen_h / 2;
    mouse_cycle = 0;
    mouse_buttons = 0;
    prev_buttons = 0;

    /* Tell the mouse to use default settings */
    ps2_write_device(2, 0xF6);
    (void)ps2_read(); /* Acknowledge 0xFA */

    /* Enable packet streaming */
    ps2_write_device(2, 0xF4);
    (void)ps2_read(); /* Acknowledge 0xFA */

    /* Unmask Cascade IRQ 2 and Mouse IRQ 12 */
    pic_unmask(IRQ_CASCADE);
    pic_unmask(IRQ_MOUSE);

    klog("[mouse] Mouse driver initialized (%dx%d)\n", screen_w, screen_h);
}

void mouse_handler(void) {
    uint8_t status = ps2_read();
    mouse_bytes[mouse_cycle++] = status;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        /* Verify bit 3 is set (sync bit in PS/2 mouse packet) */
        if (!(mouse_bytes[0] & 0x08)) {
            return;
        }

        prev_buttons = mouse_buttons;
        mouse_buttons = mouse_bytes[0] & 0x07;

        int rel_x = (int)mouse_bytes[1];
        int rel_y = (int)mouse_bytes[2];

        if (mouse_bytes[0] & 0x10) rel_x -= 256;
        if (mouse_bytes[0] & 0x20) rel_y -= 256;

        mouse_x += rel_x;
        mouse_y -= rel_y; /* Y is inverted on PS/2 mouse */

        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= max_w) mouse_x = max_w - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= max_h) mouse_y = max_h - 1;
    }
}

void mouse_get_state(int *x, int *y, int *buttons) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
    if (buttons) *buttons = mouse_buttons;
}

int mouse_was_clicked(int button) {
    return ((mouse_buttons & button) && !(prev_buttons & button));
}
