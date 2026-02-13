# Milestone: Build/CI Stabilisierung

## Umgesetzt

- Reproduzierbare Build-Umgebung über `buildenv/Dockerfile`.
- Einheitlicher Build-Einstieg über `scripts/build.sh`.
- GitHub Actions Workflow unter `.github/workflows/ci.yml` mit:
  - Container-basiertem Build (`make clean && make`)
  - Artefakt-Upload (`build/roninos.iso`, `build/roninos.elf`)
  - optionalem `qemu-smoke` Job via `workflow_dispatch`

## Nutzung

Lokal via Docker:

```bash
scripts/build.sh
```

CI:

- Build läuft bei Push auf `main` und bei Pull Requests.
- Smoke ist bewusst optional/manual geschaltet, bis ein stabiles `SMOKE: PASS` Signal im Kernel vorhanden ist.
