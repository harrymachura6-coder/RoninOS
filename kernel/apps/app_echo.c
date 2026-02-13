#include "../console.h"

int app_echo_main(int argc, char** argv) {
    int i;

    for (i = 1; i < argc; i++) {
        console_print(argv[i]);
        if (i + 1 < argc) console_putc(' ');
    }

    console_putc('\n');
    return 0;
}
