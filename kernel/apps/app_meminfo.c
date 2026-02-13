#include "../console.h"
#include "../heap.h"
#include "../mem/pmm.h"

static void print_u32(unsigned int n) {
    char buf[11];
    int i = 0;

    if (n == 0) {
        console_putc('0');
        return;
    }

    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }

    while (i > 0) {
        i--;
        console_putc(buf[i]);
    }
}

int app_meminfo_main(int argc, char** argv) {
    heap_stats_t stats;
    pmm_stats_t pmm;

    (void)argc;
    (void)argv;

    heap_get_stats(&stats);
    pmm_get_stats(&pmm);

    console_print("heap.start: ");
    print_u32((unsigned int)stats.heap_start);
    console_putc('\n');

    console_print("heap.end: ");
    print_u32((unsigned int)stats.heap_end);
    console_putc('\n');

    console_print("heap.total: ");
    print_u32((unsigned int)stats.total_bytes);
    console_putc('\n');

    console_print("heap.used: ");
    print_u32((unsigned int)stats.used_bytes);
    console_putc('\n');

    console_print("heap.free: ");
    print_u32((unsigned int)stats.free_bytes);
    console_putc('\n');

    console_print("blocks.used: ");
    print_u32((unsigned int)stats.used_blocks);
    console_putc('\n');

    console_print("blocks.free: ");
    print_u32((unsigned int)stats.free_blocks);
    console_putc('\n');

    console_print("largest.free: ");
    print_u32((unsigned int)stats.largest_free_block);
    console_putc('\n');


    console_print("pmm.frames.total: ");
    print_u32(pmm.total_frames);
    console_putc('\n');

    console_print("pmm.frames.free: ");
    print_u32(pmm.free_frames);
    console_putc('\n');

    return 0;
}
