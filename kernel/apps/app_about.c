#include "../console.h"

int app_about_main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    console_print("RoninOS kernel apps shell\n");
    console_print("Freestanding 32-bit kernel mode (no userspace).\n");
    console_print("Add apps via kernel/apps/app_*.c and register in apps.c\n");
    return 0;
}
