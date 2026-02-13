#pragma once

#include <stdint.h>

#define PAGE_PRESENT 0x001u
#define PAGE_WRITE   0x002u
#define PAGE_USER    0x004u

void paging_init(uint32_t phys_limit);
int map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void unmap_page(uint32_t virt);
uint32_t translate(uint32_t virt);
