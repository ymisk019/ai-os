#include <stdint.h>
#include "idt.h"
#include "panic.h"
#include "serial.h"
#include "io.h"
#include "pic.h"
#include "pit.h"
#include "keyboard.h"
#include "mouse.h"

typedef struct {
    uint16_t off_lo, sel;
    uint8_t  ist, type_attr;
    uint16_t off_mid;
    uint32_t off_hi, zero;
} __attribute__((packed)) idt_entry_t;

_Static_assert(sizeof(idt_entry_t) == 16, "IDT entry must be 16 bytes");

extern const uint64_t isr_table[32];
extern const uint64_t irq_table[16];

static idt_entry_t idt[256] __attribute__((aligned(16)));
volatile int idt_bp_hits = 0;

static void set_gate(int v, uint64_t handler) {
    idt[v].off_lo    = (uint16_t)handler;
    idt[v].sel       = 0x08;          /* kernel code segment from boot.S GDT */
    idt[v].ist       = 0;
    idt[v].type_attr = 0x8E;          /* present, DPL0, 64-bit interrupt gate */
    idt[v].off_mid   = (uint16_t)(handler >> 16);
    idt[v].off_hi    = (uint32_t)(handler >> 32);
    idt[v].zero      = 0;
}

void idt_init(void) {
    /* Initialize 8259 PIC controllers */
    pic_init();

    /* Set exception handlers (0-31) */
    for (int v = 0; v < 32; v++) {
        set_gate(v, isr_table[v]);
    }

    /* Set IRQ handlers (32-47) */
    for (int i = 0; i < 16; i++) {
        set_gate(32 + i, irq_table[i]);
    }

    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) idtr = {
        (uint16_t)(sizeof(idt) - 1), (uint64_t)(uintptr_t)idt
    };
    __asm__ volatile("lidt %0" :: "m"(idtr));
    klog("[idt] IDT loaded with 32 exceptions + 16 IRQ gates\n");
}

void exception_handler(regs_t *r) {
    if (r->vector == 3) { /* Breakpoint self-test */
        idt_bp_hits++;
        klog("[trap] #BP handled successfully at 0x%lx\n", (unsigned long)r->rip);
        return;
    }
    kpanic_exception(r);
}

void irq_handler(regs_t *r) {
    uint8_t irq = (uint8_t)(r->vector - 32);

    switch (irq) {
    case IRQ_TIMER:
        pit_handler();
        break;
    case IRQ_KEYBOARD:
        keyboard_handler();
        break;
    case IRQ_MOUSE:
        mouse_handler();
        break;
    default:
        break;
    }

    pic_eoi(irq);
}
