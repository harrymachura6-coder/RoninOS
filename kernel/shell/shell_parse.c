#include "shell_parse.h"
#include <stdbool.h>

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int shell_parse(char* line, char** argv, int max_argv) {
    int argc = 0;
    char* p = line;

    while (*p) {
        // skip spaces
        while (*p && is_space(*p)) p++;
        if (!*p) break;
        if (argc >= max_argv) break;

        // start of token
        if (*p == '"') {
            // quoted token
            p++; // skip opening quote
            argv[argc++] = p;

            char* out = p; // optional: handle \" by compacting
            while (*p) {
                if (*p == '\\' && p[1] == '"') { // allow \" inside quotes
                    *out++ = '"';
                    p += 2;
                    continue;
                }
                if (*p == '"') {
                    p++; // consume closing quote
                    break;
                }
                *out++ = *p++;
            }
            *out = '\0';

            // after quote: skip until space or end, null-terminate if needed
            while (*p && !is_space(*p)) {
                // Wenn du "abc"def verbieten willst, könntest du hier Fehler werfen.
                p++;
            }
        } else {
            argv[argc++] = p;
            while (*p && !is_space(*p)) p++;
            if (*p) *p++ = '\0';
        }

        // ensure separation
        while (*p && is_space(*p)) { *p = '\0'; p++; }
    }

    return argc;
}
