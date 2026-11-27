## Source layout

Every library header (`.h`) goes in `src/common/<library>/`, including private ones used by a single library. `src/libraries/` holds only `.c` sources and `meson.build` files. Include library headers by their path under `src/common/` (for example `#include "assets/stb_image.h"`); don't add `include_directories('.')` for a library directory.

`src/binaries/<binary>/` is different: each one is a self-contained sub-project that holds every file that goes into that binary, headers included (for example `testing/test_framework.h`).

## Agent skills

### Issue tracker

Issues are tracked in GitHub Issues for `brandon515/Velora` via the `gh` CLI. See `docs/agents/issue-tracker.md`.

Filing a bug: one issue per failing test, with Expected and Actual sections, and the test marked `RUN_TEST_KNOWN_BUG`. See `docs/agents/issue-tracker.md#filing-a-bug`.

### Triage labels

Uses the five default labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `GLOSSARY.md` and `docs/adr/` at the repo root. See `docs/agents/domain.md`.
