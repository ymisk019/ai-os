#include "heap.h"
#include "pmm.h"
#include "serial.h"
#include "kstring.h"
#include "panic.h"

#define HEAP_MAGIC 0x48454150 /* "HEAP" */
#define HEAP_ALIGN 16
#define HEAP_INITIAL_PAGES 256 /* 1 MB initial heap, expands dynamically */

typedef struct heap_block {
    uint32_t magic;
    uint32_t is_free;
    size_t   size;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

static heap_block_t *heap_start = NULL;
static size_t heap_used_bytes = 0;
static size_t heap_total_bytes = 0;

static void heap_expand(size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        void *page = pmm_alloc_page();
        if (!page) {
            klog("[heap] Out of physical memory for heap expansion\n");
            return;
        }
        heap_total_bytes += PAGE_SIZE;

        heap_block_t *new_block = (heap_block_t *)page;
        new_block->magic = HEAP_MAGIC;
        new_block->is_free = 1;
        new_block->size = PAGE_SIZE - sizeof(heap_block_t);
        new_block->next = NULL;
        new_block->prev = NULL;

        if (!heap_start) {
            heap_start = new_block;
        } else {
            heap_block_t *curr = heap_start;
            while (curr->next) curr = curr->next;
            curr->next = new_block;
            new_block->prev = curr;
        }
    }
}

void heap_init(void) {
    klog("[heap] Initializing Kernel Heap...\n");
    heap_expand(HEAP_INITIAL_PAGES);
    klog("[heap] Initialized with %lu KB\n", (unsigned long)(heap_total_bytes >> 10));
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1);

    heap_block_t *curr = heap_start;
    while (curr) {
        if (curr->magic != HEAP_MAGIC) {
            kpanic("Heap corruption detected at %p (invalid magic: 0x%x)", curr, curr->magic);
        }
        if (curr->is_free && curr->size >= size) {
            /* Split block if significantly larger */
            if (curr->size >= size + sizeof(heap_block_t) + 32) {
                heap_block_t *split = (heap_block_t *)((uint8_t *)curr + sizeof(heap_block_t) + size);
                split->magic = HEAP_MAGIC;
                split->is_free = 1;
                split->size = curr->size - size - sizeof(heap_block_t);
                split->next = curr->next;
                split->prev = curr;
                if (curr->next) curr->next->prev = split;
                curr->next = split;
                curr->size = size;
            }
            curr->is_free = 0;
            heap_used_bytes += curr->size;
            return (void *)((uint8_t *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    /* Need expansion */
    size_t needed_pages = (size + sizeof(heap_block_t) + PAGE_SIZE - 1) / PAGE_SIZE;
    if (needed_pages < 32) needed_pages = 32;
    heap_expand(needed_pages);

    /* Try again */
    curr = heap_start;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = 0;
            heap_used_bytes += curr->size;
            return (void *)((uint8_t *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    klog("[heap] kmalloc failed to allocate %lu bytes\n", (unsigned long)size);
    return NULL;
}

void *kcalloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = kmalloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) {
        kpanic("kfree: invalid pointer or corrupted block at %p", ptr);
    }
    if (block->is_free) return; /* double free */

    block->is_free = 1;
    if (heap_used_bytes >= block->size) {
        heap_used_bytes -= block->size;
    }

    /* Coalesce next */
    if (block->next && block->next->is_free &&
        (uint8_t *)block + sizeof(heap_block_t) + block->size == (uint8_t *)block->next) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }

    /* Coalesce prev */
    if (block->prev && block->prev->is_free &&
        (uint8_t *)block->prev + sizeof(heap_block_t) + block->prev->size == (uint8_t *)block) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) return NULL;
    if (block->size >= new_size) return ptr;

    void *new_ptr = kmalloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size < new_size ? block->size : new_size);
        kfree(ptr);
    }
    return new_ptr;
}

size_t heap_get_used(void) { return heap_used_bytes; }
size_t heap_get_total(void) { return heap_total_bytes; }
