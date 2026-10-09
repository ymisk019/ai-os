#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "serial.h"

#define KBD_BUF_SIZE 256

static char kbd_buffer[KBD_BUF_SIZE];
static int kbd_head = 0;
static int kbd_tail = 0;
static int shift_down = 0;
static int caps_lock = 0;

static const char scancodes_normal[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Ctrl */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, /* Right Shift */
    '*',
    0, /* Alt */
    ' ', /* Space */
    0, /* Caps Lock */
};

static const char scancodes_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, /* Ctrl */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, /* Left Shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, /* Right Shift */
    '*',
    0, /* Alt */
    ' ', /* Space */
    0, /* Caps Lock */
};

void keyboard_init(void) {
    kbd_head = 0;
    kbd_tail = 0;
    shift_down = 0;
    caps_lock = 0;
    pic_unmask(IRQ_KEYBOARD);
    klog("[kbd] Keyboard driver initialized\n");
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_down = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_down = 0;
        return;
    }
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    if (scancode & 0x80) {
        /* Key release */
        return;
    }

    if (scancode < 128) {
        char ch = 0;
        int use_shift = shift_down;
        if (caps_lock && scancodes_normal[scancode] >= 'a' && scancodes_normal[scancode] <= 'z') {
            use_shift = !use_shift;
        }

        if (use_shift) {
            ch = scancodes_shift[scancode];
        } else {
            ch = scancodes_normal[scancode];
        }

        if (ch) {
            int next = (kbd_head + 1) % KBD_BUF_SIZE;
            if (next != kbd_tail) {
                kbd_buffer[kbd_head] = ch;
                kbd_head = next;
            }
        }
    }
}

int keyboard_has_char(void) {
    return kbd_head != kbd_tail;
}

char keyboard_getchar(void) {
    if (kbd_head == kbd_tail) return 0;
    char ch = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return ch;
}
