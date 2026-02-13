#include "../console.h"
#include "../sched/thread.h"

#include <stdint.h>

static unsigned g_worker_ticks;

static int parse_u32(const char* s, unsigned int* out) {
    unsigned int v = 0;
    int seen = 0;

    while (*s) {
        char c = *s;
        if (c < '0' || c > '9') {
            return 0;
        }
        seen = 1;
        v = v * 10u + (unsigned int)(c - '0');
        s++;
    }

    if (!seen) return 0;
    *out = v;
    return 1;
}

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

static void worker_entry(void* arg) {
    int tid = (int)(uintptr_t)arg;
    unsigned int local_tick = 0;

    for (;;) {
        local_tick++;
        if ((local_tick % 50u) == 0u) {
            console_print("[T");
            print_u32((unsigned int)tid);
            console_print("] tick ");
            print_u32(local_tick / 50u);
            console_putc('\n');
        }
        g_worker_ticks++;
        thread_yield();
    }
}

int app_spawn_main(int argc, char** argv) {
    unsigned int count;
    unsigned int i;

    if (argc < 2 || !parse_u32(argv[1], &count)) {
        console_print("usage: spawn <n>\n");
        return 1;
    }

    for (i = 0; i < count; i++) {
        int tid = thread_create("worker", worker_entry, (void*)(uintptr_t)(i + 1u));
        if (tid < 0) {
            console_print("spawn failed at thread ");
            print_u32(i + 1u);
            console_putc('\n');
            return 1;
        }
    }

    console_print("spawned ");
    print_u32(count);
    console_print(" thread(s)\n");
    return 0;
}

int app_yield_main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    thread_yield();
    return 0;
}

int app_ps_main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    sched_dump();
    return 0;
}

int app_preempt_main(int argc, char** argv) {
    int enabled;

    if (argc < 2) {
        console_print("usage: preempt on|off\n");
        return 1;
    }

    if (argv[1][0] == 'o' && argv[1][1] == 'n' && argv[1][2] == 0) {
        enabled = 1;
    } else if (argv[1][0] == 'o' && argv[1][1] == 'f' && argv[1][2] == 'f' && argv[1][3] == 0) {
        enabled = 0;
    } else {
        console_print("usage: preempt on|off\n");
        return 1;
    }

    sched_set_preempt(enabled);
    console_print("preempt ");
    console_print(enabled ? "on\n" : "off\n");
    return 0;
}
