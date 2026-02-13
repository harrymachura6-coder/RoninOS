# AGENTS.md

## OwnBoot Regeln
- Freestanding Kernel: keine libc verwenden, außer eigene Implementierung in kernel/lib/.
- Große änderungen sind erlaubt, keine neuen Dependencies.
- Nach Änderungen: `make clean && make`.
- Shell/Apps: neue Programme als kernel/apps/app_*.c + Registrierung in apps.c.

