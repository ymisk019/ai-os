#pragma once
#include <stdint.h>
#include <stddef.h>
#include "bootinfo.h"

#define PAGE_SIZE 4096

void     pmm_init(const bootinfo_t *bi);
void    *pmm_alloc_page(void);
void     pmm_free_page(void *p);
uint64_t pmm_get_total_ram(void);
uint64_t pmm_get_usable_ram(void);
uint64_t pmm_get_used_ram(void);
uint64_t pmm_get_free_ram(void);
