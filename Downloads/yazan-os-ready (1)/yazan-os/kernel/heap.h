#pragma once
#include <stddef.h>
#include <stdint.h>

void  heap_init(void);
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t new_size);
void  kfree(void *ptr);

size_t heap_get_used(void);
size_t heap_get_total(void);
