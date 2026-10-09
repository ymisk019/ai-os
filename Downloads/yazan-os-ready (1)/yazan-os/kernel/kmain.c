/* Yazan OS 3.0 (Fire Edition) - Kernel Entry & Event Loop
 * Bootstraps memory, interrupts, drivers, splash screen, and the Desktop GUI. */
#include <stdint.h>
#include "mb2.h"
#include "bootinfo.h"
#include "gfx.h"
#include "serial.h"
#include "vgatext.h"
#include "panic.h"
#include "io.h"
#include "kstring.h"
#include "pmm.h"
#include "heap.h"
#include "idt.h"
#include "pit.h"
#include "ps2.h"
#include "keyboard.h"
#include "mouse.h"
#include "bootsplash.h"
#include "../gui/desktop.h"
#include "../gui/wm.h"

#ifndef TEST_FAULT
#define TEST_FAULT 0
#endif

extern char __kernel_start[], __kernel_end[];

static void cpu_vendor(char out[13]) {
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0));
    memcpy(out + 0, &b, 4);
    memcpy(out + 4, &d, 4);
    memcpy(out + 8, &c, 4);
    out[12] = 0;
}

static void parse_mb2(uintptr_t mbi, bootinfo_t *bi, gfx_fb_t *fb) {
    const uint8_t *p = (const uint8_t *)mbi + 8;
    for (;;) {
        const mb2_tag_t *t = (const mb2_tag_t *)p;
        if (t->type == MB2_TAG_END) break;

        if (t->type == MB2_TAG_LOADER) {
            const char *s = (const char *)(p + 8);
            size_t n = t->size - 8;
            if (n > sizeof(bi->loader)) n = sizeof(bi->loader);
            size_t i = 0;
            for (; i + 1 < n && s[i]; i++) bi->loader[i] = s[i];
            bi->loader[i] = 0;
        } else if (t->type == MB2_TAG_MMAP) {
            const mb2_mmap_tag_t *m = (const mb2_mmap_tag_t *)t;
            bi->mmap_addr = (uintptr_t)(p + sizeof(*m));
            bi->mmap_size = t->size - sizeof(*m);
            bi->mmap_entry_size = m->entry_size;

            const uint8_t *e = p + sizeof(*m), *end = p + t->size;
            for (; e + m->entry_size <= end; e += m->entry_size) {
                const mb2_mmap_entry_t *me = (const mb2_mmap_entry_t *)e;
                bi->mem_total += me->len;
                if (me->type == MB2_MMAP_AVAILABLE) {
                    bi->mem_usable += me->len;
                }
            }
        } else if (t->type == MB2_TAG_FB) {
            const mb2_fb_tag_t *f = (const mb2_fb_tag_t *)t;
            bi->fb_w = f->width;
            bi->fb_h = f->height;
            bi->fb_bpp = f->bpp;
            if (f->type == MB2_FB_RGB && (f->bpp == 32 || f->bpp == 24 || f->bpp == 16)) {
                fb->base = (uint8_t *)(uintptr_t)f->addr;
                fb->pitch = f->pitch;
                fb->width = f->width;
                fb->height = f->height;
                fb->bpp = f->bpp;
                fb->rpos = f->red_pos;   fb->rsize = f->red_size;
                fb->gpos = f->green_pos; fb->gsize = f->green_size;
                fb->bpos = f->blue_pos;  fb->bsize = f->blue_size;
                bi->fb_ok = 1;
            }
        }
        p += (t->size + 7u) & ~7u;
    }
}

static void test_fault_if_requested(void) {
#if TEST_FAULT == 1
    klog("TEST: triggering divide-by-zero (#DE)\n");
    volatile int zero = 0; volatile int q = 1 / zero; (void)q;
#elif TEST_FAULT == 2
    klog("TEST: executing invalid opcode (#UD)\n");
    __asm__ volatile("ud2");
#elif TEST_FAULT == 3
    klog("TEST: touching unmapped memory (#PF)\n");
    *(volatile uint64_t *)0x180000000ULL = 1;
#elif TEST_FAULT == 4
    klog("TEST: non-canonical address access (#GP)\n");
    *(volatile uint64_t *)0xFFFF800000000000ULL = 1;
#endif
}

void kmain(uintptr_t mbi, uint32_t magic) {
    serial_init();
    klog("\n=========================================\n");
    klog("    YAZAN OS 3.0 - FIRE EDITION (x86_64) \n");
    klog("=========================================\n");

    if (magic != MB2_BOOT_MAGIC) {
        kpanic("Bad Multiboot2 magic: 0x%x", magic);
    }

    bootinfo_t bi;
    memset(&bi, 0, sizeof(bi));
    gfx_fb_t fb;
    memset(&fb, 0, sizeof(fb));

    bi.kernel_start = (uintptr_t)__kernel_start;
    bi.kernel_end   = (uintptr_t)__kernel_end;
    cpu_vendor(bi.cpu_vendor);
    parse_mb2(mbi, &bi, &fb);

    if (bi.fb_ok) {
        gfx_init(&fb);
    }

    /* 1. Physical Memory Manager (Checks minimum 64MB RAM) */
    pmm_init(&bi);

    /* 2. Kernel Heap Allocator */
    heap_init();

    /* 3. Interrupts & IDT (Exceptions + Hardware IRQs) */
    idt_init();

    /* 4. Programmable Interval Timer (100 Hz) */
    pit_init(100);

    /* 5. PS/2 Input Devices (Keyboard & Mouse) */
    ps2_init();
    keyboard_init();
    mouse_init(bi.fb_w > 0 ? bi.fb_w : 1024, bi.fb_h > 0 ? bi.fb_h : 768);

    /* Enable interrupts */
    sti();

    /* Optional fault injection for testing */
    test_fault_if_requested();

    if (bi.fb_ok) {
        /* Beautiful Boot Splash with Loading Progress & Verification */
        bootsplash_show(&bi);

        /* Enable Double Buffering for 60 FPS flicker-free graphics */
        gfx_enable_double_buffer();

        /* Initialize Windows + macOS + Android Hybrid Desktop GUI */
        desktop_init(bi.fb_w, bi.fb_h);

        /* Main GUI Event Loop */
        int mx = bi.fb_w / 2, my = bi.fb_h / 2, btns = 0;
        int prev_left_btn = 0;

        for (;;) {
            /* 1. Handle Keyboard */
            while (keyboard_has_char()) {
                char ch = keyboard_getchar();
                wm_handle_key(ch);
            }

            /* 2. Handle Mouse */
            mouse_get_state(&mx, &my, &btns);
            int clicked = (btns & 1) && !prev_left_btn;
            prev_left_btn = (btns & 1);

            desktop_handle_mouse(mx, my, btns, clicked);

            /* 3. Render Desktop Scene to Back Buffer */
            desktop_render();

            /* 4. Render Mouse Pointer */
            gfx_draw_cursor(mx, my);

            /* 5. Flip Back Buffer to Screen */
            gfx_flip();

            /* Smooth frame pacing */
            pit_sleep_ms(15);
        }
    } else {
        klog("[boot] No RGB framebuffer, using VGA text fallback\n");
        vga_clear(0x1F);
        vga_puts_at(2, 1, "YAZAN OS 3.0 (Fire Edition) - VGA Text Fallback", 0x1F);
        vga_puts_at(2, 3, "See the serial log COM1 for detailed diagnostics.", 0x1F);
        cpu_halt_forever();
    }
}
