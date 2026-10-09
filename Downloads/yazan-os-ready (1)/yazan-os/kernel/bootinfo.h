#pragma once
#include <stdint.h>

#define MIN_REQUIRED_RAM_MB 64ULL

typedef struct {
    char      loader[64];
    char      cpu_vendor[16];
    uint64_t  mem_usable;          /* bytes of RAM marked available by firmware */
    uint64_t  mem_total;           /* total detected RAM in bytes */
    uint32_t  fb_w, fb_h, fb_bpp;
    int       fb_ok;
    uintptr_t kernel_start, kernel_end;
    uintptr_t mmap_addr;
    uint32_t  mmap_size;
    uint32_t  mmap_entry_size;
} bootinfo_t;
