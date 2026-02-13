#include "shell_commands.h"
#include "shell_parse.h"
#include "console.h"
#include "../lib/string.h"

static const command_t* find_command(const char* name) {
    for (size_t i = 0; i < g_command_count; i++) {
        if (strcmp(name, g_commands[i].name) == 0) return &g_commands[i];
    }
    return 0;
}

void shell_run_line(char* line) {
    char* argv[16];
    int argc = shell_parse(line, argv, 16);

    if (argc == 0) return;

    const command_t* cmd = find_command(argv[0]);
    if (!cmd) {
        console_write("Unknown command. Type 'help'.\n");
        return;
    }

    cmd->fn(argc, argv);
}
