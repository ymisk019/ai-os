/* Yazan OS 3.0 - Red Screen of Death (RSOD) & Kernel Panic
 * Handles hardware errors, insufficient RAM errors, and CPU exceptions. */
#include "panic.h"
#include "gfx.h"
#include "serial.h"
#include "vgatext.h"
#include "kstring.h"
#include "io.h"
#include "theme.h"

static const char *const exc_names[32] = {
    "Divide Error (#DE)", "Debug (#DB)", "Non-Maskable Interrupt (NMI)", "Breakpoint (#BP)",
    "Overflow (#OF)", "Bound Range Exceeded (#BR)", "Invalid Opcode (#UD)", "Device Not Available (#NM)",
    "Double Fault (#DF)", "Coprocessor Segment Overrun", "Invalid TSS (#TS)", "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)", "General Protection Fault (#GP)", "Page Fault (#PF)", "Reserved",
    "x87 Floating-Point (#MF)", "Alignment Check (#AC)", "Machine Check (#MC)", "SIMD Floating-Point (#XM)",
    "Virtualization (#VE)", "Control Protection (#CP)", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection (#HV)", "VMM Communication (#VC)", "Security (#SX)", "Reserved",
};

static int backtrace(uint64_t rbp, uint64_t *out, int max) {
    int n = 0;
    while (n < max && rbp >= 0x100000 && rbp < 0x100000000ULL && !(rbp & 7)) {
        const uint64_t *f = (const uint64_t *)(uintptr_t)rbp;
        uint64_t ret = f[1], next = f[0];
        if (!ret) break;
        out[n++] = ret;
        if (next <= rbp) break;
        rbp = next;
    }
    return n;
}

void kpanic(const char *fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    klog("\n*** KERNEL PANIC: %s ***\n", msg);

    if (gfx_ready()) {
        int W = (int)gfx_width(), H = (int)gfx_height();
        gfx_vgradient(0, 0, W, H, PANIC_BG_TOP, PANIC_BG_BOTTOM);

        int scale = W >= 900 ? 2 : 1;
        int x = 40, y = 40;

        gfx_text(x, y, ":( YAZAN OS ENCOUNTERED A PROBLEM", scale + 1, PANIC_TEXT_WHITE);
        y += 8 * (scale + 1) + 20;

        gfx_text(x, y, "A critical system error occurred and the OS had to stop.", scale, PANIC_TEXT_MUTED);
        y += 8 * scale + 14;

        gfx_text(x, y, "Message:", scale, PANIC_TEXT_YELLOW);
        y += 8 * scale + 6;

        gfx_fill_rrect(x, y, W - 80, 50 * scale, 8, 0x300505);
        gfx_rect_outline(x, y, W - 80, 50 * scale, 0x991B1B);
        gfx_text(x + 16, y + 16, msg, scale, PANIC_TEXT_WHITE);
        y += 50 * scale + 24;

        gfx_text(x, y, "Technical Information:", scale, PANIC_TEXT_YELLOW);
        y += 8 * scale + 8;
        gfx_text(x, y, "Stop Code: KERNEL_HALT_EXCEPTION (0xDEAD0001)", scale, PANIC_TEXT_WHITE);
        y += 8 * scale + 6;
        gfx_text(x, y, "Troubleshooting: Please check logs or restart the machine.", scale, PANIC_TEXT_MUTED);

        gfx_flip();
    } else {
        vga_clear(0x4F);
        vga_puts_at(2, 2, "YAZAN OS - KERNEL PANIC", 0x4F);
        vga_puts_at(2, 4, msg, 0x4F);
    }
    cpu_halt_forever();
}

void kpanic_ram_error(uint64_t detected_mb, uint64_t required_mb) {
    klog("\n=======================================================\n");
    klog("CRITICAL ERROR: INSUFFICIENT SYSTEM RAM!\n");
    klog("Detected: %lu MB | Required: %lu MB\n", (unsigned long)detected_mb, (unsigned long)required_mb);
    klog("=======================================================\n");

    if (gfx_ready()) {
        int W = (int)gfx_width(), H = (int)gfx_height();
        gfx_vgradient(0, 0, W, H, PANIC_BG_TOP, PANIC_BG_BOTTOM);

        int scale = W >= 900 ? 2 : 1;
        int x = 40, y = 35;

        /* Sad face & header */
        gfx_text(x, y, ":(  YAZAN OS - SYSTEM HALTED", scale + 1, PANIC_TEXT_WHITE);
        y += 8 * (scale + 1) + 16;

        gfx_text(x, y, "Your computer ran into a critical hardware memory error.", scale, PANIC_TEXT_MUTED);
        y += 8 * scale + 18;

        /* Red diagnostic box */
        int box_h = 160 * scale;
        gfx_fill_rrect(x, y, W - 80, box_h, 8, 0x380505);
        gfx_rect_outline(x, y, W - 80, box_h, 0xDC2626);

        int by = y + 16;
        gfx_text(x + 20, by, "STOP CODE: INSUFFICIENT_SYSTEM_RAM (0x0000003E)", scale, PANIC_TEXT_YELLOW);
        by += 8 * scale + 12;

        char ram_line[128];
        ksnprintf(ram_line, sizeof ram_line, "Detected RAM : %lu MB", (unsigned long)detected_mb);
        gfx_text(x + 20, by, ram_line, scale, PANIC_TEXT_WHITE);
        by += 8 * scale + 8;

        ksnprintf(ram_line, sizeof ram_line, "Required RAM : %lu MB Minimum", (unsigned long)required_mb);
        gfx_text(x + 20, by, ram_line, scale, 0x4ADE80);
        by += 8 * scale + 14;

        gfx_text(x + 20, by, "Arabic Diagnostics:", scale, PANIC_TEXT_YELLOW);
        by += 8 * scale + 8;

        gfx_text(x + 20, by, "Khata'a: Al-Thakira al-Ashwa'iya (RAM) ghayr kafiya!", scale, PANIC_TEXT_WHITE);
        by += 8 * scale + 6;
        gfx_text(x + 20, by, "Yurja ziyadat RAM fi i'dadat QEMU aw VirtualBox ila 128MB+.", scale, PANIC_TEXT_MUTED);

        y += box_h + 20;

        /* Troubleshooting advice */
        gfx_text(x, y, "How to fix this issue:", scale, PANIC_TEXT_YELLOW);
        y += 8 * scale + 8;
        gfx_text(x, y, "1. In QEMU: Add '-m 256M' or higher to your launch command.", scale, PANIC_TEXT_WHITE);
        y += 8 * scale + 6;
        gfx_text(x, y, "2. In VirtualBox: Open VM Settings -> System -> Motherboard -> Base Memory -> Set >= 256 MB.", scale, PANIC_TEXT_WHITE);
        y += 8 * scale + 6;
        gfx_text(x, y, "3. Ensure your motherboard doesn't reserve too much RAM for integrated GPU.", scale, PANIC_TEXT_MUTED);

        gfx_flip();
    } else {
        vga_clear(0x4F);
        vga_puts_at(2, 2, "YAZAN OS - CRITICAL: INSUFFICIENT RAM", 0x4F);
        vga_puts_at(2, 4, "Minimum 64 MB required for Yazan OS to operate.", 0x4F);
    }

    cpu_halt_forever();
}

void panic_exception_screen(const regs_t *r, uint64_t cr2, uint64_t cr3) {
    const char *name = r->vector < 32 ? exc_names[r->vector] : "Unknown Exception";
    char l[8][96];
    int nl = 0;

    ksnprintf(l[nl++], sizeof l[0], "%s (vector %lu, err 0x%lx)",
              name, (unsigned long)r->vector, (unsigned long)r->error);

    if (r->vector == 14) {
        ksnprintf(l[nl++], sizeof l[0], "%s of %s page at 0x%lx (%s mode)",
                  (r->error & 2) ? "Write" : "Read",
                  (r->error & 1) ? "protected" : "non-present",
                  (unsigned long)cr2,
                  (r->error & 4) ? "user" : "kernel");
    }

    uint64_t bt[8];
    int nbt = backtrace(r->rbp, bt, 8);

    klog("\n*** KERNEL PANIC: %s ***\n", l[0]);
    for (int i = 1; i < nl; i++) klog("%s\n", l[i]);
    klog("RIP=%016lx RSP=%016lx RFLAGS=%016lx\n", (unsigned long)r->rip, (unsigned long)r->rsp, (unsigned long)r->rflags);

    if (!gfx_ready()) {
        vga_clear(0x4F);
        vga_puts_at(2, 1, "YAZAN OS - KERNEL EXCEPTION", 0x4F);
        vga_puts_at(2, 3, l[0], 0x4F);
        return;
    }

    int W = (int)gfx_width(), H = (int)gfx_height();
    int s = W >= 900 ? 2 : 1, lh = 8 * s + 6, x = 32, y = 20;

    gfx_vgradient(0, 0, W, H, PANIC_BG_TOP, PANIC_BG_BOTTOM);

    gfx_text(x, y, ":(  YAZAN OS - KERNEL PANIC", s + 1, PANIC_TEXT_WHITE);
    y += 8 * (s + 1) + 12;

    gfx_text(x, y, "System halted to prevent damage to your filesystem.", s, PANIC_TEXT_MUTED);
    y += lh + 8;

    for (int i = 0; i < nl; i++) {
        gfx_text(x, y, l[i], s, PANIC_TEXT_YELLOW);
        y += lh;
    }
    y += 8;

    char line[96];
#define REGPAIR(an, av, bn, bv) do { \
        ksnprintf(line, sizeof line, an "=%016lx  " bn "=%016lx", (unsigned long)(av), (unsigned long)(bv)); \
        gfx_text(x, y, line, s, PANIC_TEXT_WHITE); y += lh; } while (0)
    REGPAIR("RAX", r->rax, "RBX", r->rbx);
    REGPAIR("RCX", r->rcx, "RDX", r->rdx);
    REGPAIR("RSI", r->rsi, "RDI", r->rdi);
    REGPAIR("RBP", r->rbp, "RSP", r->rsp);
    REGPAIR("R8 ", r->r8,  "R9 ", r->r9);
    REGPAIR("R10", r->r10, "R11", r->r11);
    REGPAIR("R12", r->r12, "R13", r->r13);
    REGPAIR("R14", r->r14, "R15", r->r15);
    REGPAIR("RIP", r->rip, "FLG", r->rflags);
    REGPAIR("CS ", r->cs,  "SS ", r->ss);
    REGPAIR("CR2", cr2,    "CR3", cr3);
#undef REGPAIR

    y += 8;
    gfx_text(x, y, "Call Stack Backtrace:", s, PANIC_TEXT_YELLOW); y += lh;
    for (int i = 0; i < nbt; i++) {
        ksnprintf(line, sizeof line, "  #%d  0x%016lx", i, (unsigned long)bt[i]);
        gfx_text(x, y, line, s, PANIC_TEXT_WHITE); y += lh;
    }

    gfx_flip();
}

void kpanic_exception(const regs_t *r) {
    uint64_t cr2, cr3;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    panic_exception_screen(r, cr2, cr3);
    cpu_halt_forever();
}
