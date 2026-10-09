#include "ps2.h"
#include "io.h"
#include "serial.h"

static inline void ps2_wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS) & 0x02)) return;
    }
}

static inline void ps2_wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS) & 0x01) return;
    }
}

uint8_t ps2_read(void) {
    ps2_wait_read();
    return inb(PS2_DATA);
}

void ps2_write(uint8_t port, uint8_t val) {
    ps2_wait_write();
    outb(port, val);
}

void ps2_write_device(uint8_t dev, uint8_t val) {
    if (dev == 2) {
        ps2_write(PS2_CMD, 0xD4); /* write next byte to second PS/2 port */
    }
    ps2_write(PS2_DATA, val);
}

void ps2_init(void) {
    klog("[ps2] Initializing PS/2 controller...\n");

    /* Disable devices */
    ps2_write(PS2_CMD, 0xAD);
    ps2_write(PS2_CMD, 0xA7);

    /* Flush output buffer */
    while (inb(PS2_STATUS) & 0x01) {
        inb(PS2_DATA);
    }

    /* Read Controller Configuration Byte */
    ps2_write(PS2_CMD, 0x20);
    uint8_t config = ps2_read();

    /* Enable IRQs (bit 0 = keyboard IRQ 1, bit 1 = mouse IRQ 12) */
    config |= 0x03;
    config &= ~0x40; /* Disable translation if needed, or keep standard */

    /* Write back config */
    ps2_write(PS2_CMD, 0x60);
    ps2_write(PS2_DATA, config);

    /* Enable devices */
    ps2_write(PS2_CMD, 0xAE); /* Enable keyboard */
    ps2_write(PS2_CMD, 0xA8); /* Enable mouse */

    klog("[ps2] PS/2 controller ready\n");
}
