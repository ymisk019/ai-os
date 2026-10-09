#include "pmm.h"
#include "panic.h"
#include "serial.h"
#include "kstring.h"
#include "mb2.h"

#define BITMAP_SET(bm, bit)   ((bm)[(bit) / 64] |= (1ULL << ((bit) % 64)))
#define BITMAP_CLEAR(bm, bit) ((bm)[(bit) / 64] &= ~(1ULL << ((bit) % 64)))
#define BITMAP_TEST(bm, bit)  (((bm)[(bit) / 64] & (1ULL << ((bit) % 64))) != 0)

#define MAX_PHYS_PAGES (4ULL * 1024 * 1024 * 1024 / PAGE_SIZE) /* 4 GiB identity mapped */
#define BITMAP_WORDS   (MAX_PHYS_PAGES / 64)

static uint64_t bitmap[BITMAP_WORDS];
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t used_pages = 0;
static uint64_t total_ram_bytes = 0;
static uint64_t usable_ram_bytes = 0;

static void mark_region_used(uintptr_t start, uintptr_t end) {
    uintptr_t p_start = start / PAGE_SIZE;
    uintptr_t p_end = (end + PAGE_SIZE - 1) / PAGE_SIZE;
    if (p_end > total_pages) p_end = total_pages;

    for (uintptr_t i = p_start; i < p_end; i++) {
        if (!BITMAP_TEST(bitmap, i)) {
            BITMAP_SET(bitmap, i);
            if (free_pages > 0) free_pages--;
            used_pages++;
        }
    }
}

static void mark_region_free(uintptr_t start, uintptr_t end) {
    uintptr_t p_start = (start + PAGE_SIZE - 1) / PAGE_SIZE;
    uintptr_t p_end = end / PAGE_SIZE;
    if (p_end > total_pages) p_end = total_pages;

    for (uintptr_t i = p_start; i < p_end; i++) {
        if (BITMAP_TEST(bitmap, i)) {
            BITMAP_CLEAR(bitmap, i);
            free_pages++;
            if (used_pages > 0) used_pages--;
        }
    }
}

void pmm_init(const bootinfo_t *bi) {
    klog("[pmm] Initializing Physical Memory Manager...\n");

    usable_ram_bytes = bi->mem_usable;
    total_ram_bytes = bi->mem_total > bi->mem_usable ? bi->mem_total : bi->mem_usable;

    /* Check RAM requirement: If less than required, trigger Red Screen of Death! */
    uint64_t usable_mb = usable_ram_bytes / (1024 * 1024);
    if (usable_mb < MIN_REQUIRED_RAM_MB) {
        klog("[pmm] CRITICAL: Usable RAM %lu MB is less than minimum required %lu MB!\n",
             (unsigned long)usable_mb, (unsigned long)MIN_REQUIRED_RAM_MB);
        kpanic_ram_error(usable_mb, MIN_REQUIRED_RAM_MB);
    }

    uint64_t max_addr = 0;
    if (bi->mmap_addr && bi->mmap_size) {
        uintptr_t p = bi->mmap_addr;
        uintptr_t end = bi->mmap_addr + bi->mmap_size;
        while (p < end) {
            const mb2_mmap_entry_t *me = (const mb2_mmap_entry_t *)p;
            if (me->base + me->len > max_addr) {
                max_addr = me->base + me->len;
            }
            p += bi->mmap_entry_size;
        }
    } else {
        max_addr = bi->mem_usable;
    }

    if (max_addr > 4ULL * 1024 * 1024 * 1024) {
        max_addr = 4ULL * 1024 * 1024 * 1024;
    }

    total_pages = max_addr / PAGE_SIZE;
    if (total_pages > MAX_PHYS_PAGES) total_pages = MAX_PHYS_PAGES;

    /* By default mark everything as used */
    memset(bitmap, 0xFF, sizeof(bitmap));
    free_pages = 0;
    used_pages = total_pages;

    /* Free regions reported as available */
    if (bi->mmap_addr && bi->mmap_size) {
        uintptr_t p = bi->mmap_addr;
        uintptr_t end = bi->mmap_addr + bi->mmap_size;
        while (p < end) {
            const mb2_mmap_entry_t *me = (const mb2_mmap_entry_t *)p;
            if (me->type == MB2_MMAP_AVAILABLE && me->base < 4ULL * 1024 * 1024 * 1024) {
                uintptr_t start = me->base;
                uintptr_t finish = me->base + me->len;
                if (finish > 4ULL * 1024 * 1024 * 1024) finish = 4ULL * 1024 * 1024 * 1024;
                mark_region_free(start, finish);
            }
            p += bi->mmap_entry_size;
        }
    } else {
        mark_region_free(0x100000, max_addr);
    }

    /* Protect lower 1 MiB (BIOS, IVT, BDA, VGA) */
    mark_region_used(0, 0x100000);

    /* Protect Kernel space */
    mark_region_used(bi->kernel_start, bi->kernel_end + 0x10000);

    klog("[pmm] PMM initialized: Total RAM: %lu MB, Usable: %lu MB, Free Pages: %lu\n",
         (unsigned long)(total_ram_bytes >> 20), (unsigned long)usable_mb, (unsigned long)free_pages);
}

void *pmm_alloc_page(void) {
    for (uint64_t w = 0; w < (total_pages / 64); w++) {
        if (bitmap[w] != 0xFFFFFFFFFFFFFFFFULL) {
            for (int b = 0; b < 64; b++) {
                if (!(bitmap[w] & (1ULL << b))) {
                    uint64_t page_idx = w * 64 + b;
                    if (page_idx >= total_pages) return NULL;
                    BITMAP_SET(bitmap, page_idx);
                    free_pages--;
                    used_pages++;
                    void *ptr = (void *)(uintptr_t)(page_idx * PAGE_SIZE);
                    memset(ptr, 0, PAGE_SIZE);
                    return ptr;
                }
            }
        }
    }
    klog("[pmm] WARNING: Physical memory exhausted!\n");
    return NULL;
}

void pmm_free_page(void *p) {
    uintptr_t addr = (uintptr_t)p;
    if (addr % PAGE_SIZE != 0) return;
    uint64_t page_idx = addr / PAGE_SIZE;
    if (page_idx < total_pages) {
        if (BITMAP_TEST(bitmap, page_idx)) {
            BITMAP_CLEAR(bitmap, page_idx);
            free_pages++;
            used_pages--;
        }
    }
}

uint64_t pmm_get_total_ram(void) { return total_ram_bytes; }
uint64_t pmm_get_usable_ram(void) { return usable_ram_bytes; }
uint64_t pmm_get_used_ram(void) { return used_pages * PAGE_SIZE; }
uint64_t pmm_get_free_ram(void) { return free_pages * PAGE_SIZE; }
