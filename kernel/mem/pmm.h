#pragma once

#include <stdint.h>

#define PMM_FRAME_SIZE 4096u

typedef struct pmm_stats {
    uint32_t total_frames;
    uint32_t free_frames;
    uint32_t used_frames;
    uint32_t managed_bytes;
} pmm_stats_t;

int pmm_init(uint32_t mb_magic, uint32_t mb_info_addr);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys_addr);
void pmm_get_stats(pmm_stats_t* out);
void pmm_dump_stats(void);
uint32_t pmm_get_max_phys_addr(void);
