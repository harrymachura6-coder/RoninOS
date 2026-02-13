#include "../console.h"
#include "../heap.h"
#include "../panic.h"
#include "../lib/string.h"

static int g_failures = 0;

static void ok(const char* msg) {
    console_print("[ok] ");
    console_print(msg);
    console_putc('\n');
}

static void fail(const char* msg) {
    g_failures++;
    console_print("[fail] ");
    console_print(msg);
    console_putc('\n');
}

static void expect(int cond, const char* msg) {
    if (cond) {
        ok(msg);
    } else {
        fail(msg);
    }
}

int app_heap_test_main(int argc, char** argv) {
    heap_stats_t before;
    heap_stats_t after;
    char* p1;
    char* p2;
    char* p3;

    (void)argc;
    (void)argv;

    g_failures = 0;
    heap_get_stats(&before);

    p1 = (char*)kmalloc(64);
    expect(p1 != 0, "kmalloc(64)");

    p2 = (char*)kmalloc(256);
    expect(p2 != 0, "kmalloc(256)");

    if (p1) memset(p1, 0xAA, 64);
    if (p2) memset(p2, 0xBB, 256);

    p3 = (char*)krealloc(p2, 512);
    expect(p3 != 0, "krealloc(256->512)");

    if (p3) {
        expect((unsigned char)p3[0] == 0xBB, "krealloc preserves first byte");
        expect((unsigned char)p3[10] == 0xBB, "krealloc preserves data");
    }

    kfree(p1);
    kfree(p3);

    heap_get_stats(&after);
    expect(after.free_bytes >= before.free_bytes, "free bytes recovered");

    if (g_failures == 0) {
        console_print("heap_test: PASS\n");
    } else {
        console_print("heap_test: FAIL\n");
        assert(g_failures == 0);
    }

    return g_failures == 0 ? 0 : 1;
}
