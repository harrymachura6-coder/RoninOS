#pragma once

// Gibt argc zurück, füllt argv (max_argv Einträge).
// Modifiziert 'line' in-place.
int shell_parse(char* line, char** argv, int max_argv);
