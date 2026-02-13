#include "shell_commands.h"
#include "console.h"   // kprint/kprintf/cls o.ä.
#include "../lib/string.h"    // strcmp etc.

int cmd_clear(int argc, char** argv) {
    (void)argc; (void)argv;
    console_clear();   // deine Funktion
    return 0;
}

int cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        console_write(argv[i]);
        if (i + 1 < argc) console_write(" ");
    }
    console_write("\n");
    return 0;
}

int cmd_help(int argc, char** argv) {
    (void)argc; (void)argv;
    console_write("Commands:\n");
    for (size_t i = 0; i < g_command_count; i++) {
        console_write("  ");
        console_write(g_commands[i].name);
        console_write(" - ");
        console_write(g_commands[i].desc);
        console_write("\n");
    }
    return 0;
}

const command_t g_commands[] = {
    {"help",  "List commands",            cmd_help},
    {"clear", "Clear the screen",         cmd_clear},
    {"echo",  "Print text (supports quotes)", cmd_echo},
};

const size_t g_command_count = sizeof(g_commands) / sizeof(g_commands[0]);
