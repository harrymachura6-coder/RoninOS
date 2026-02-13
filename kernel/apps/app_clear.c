#include "../console.h"

int app_clear_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_clear(0x87);
    return 0;
}
