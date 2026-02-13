#pragma once

#include "lib/types.h"

typedef struct {
    size_t heap_start;
    size_t heap_end;
    size_t total_bytes;
    size_t used_bytes;
    size_t free_bytes;
    size_t free_blocks;
    size_t used_blocks;
    size_t largest_free_block;
} heap_stats_t;

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t new_size);
void heap_get_stats(heap_stats_t* out);
