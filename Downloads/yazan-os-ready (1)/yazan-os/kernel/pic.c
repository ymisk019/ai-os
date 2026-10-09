#include "pic.h"
#include "io.h"

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

void pic_init(void) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);
    (void)a1; (void)a2;

    /* Start initialization sequence */
    outb(PIC1_CMD, ICW1_INIT);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT);
    io_wait();

    /* Vector offsets: 32 (0x20) for Master, 40 (0x28) for Slave */
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    /* Cascade configuration */
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    /* Mode: 8086/88 */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Mask all initially */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}

void pic_mask(uint8_t irq) {
    uint16_t port;
    uint8_t val;
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    val = inb(port) | (1 << irq);
    outb(port, val);
}

void pic_unmask(uint8_t irq) {
    uint16_t port;
    uint8_t val;
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    val = inb(port) & ~(1 << irq);
    outb(port, val);
}

void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
