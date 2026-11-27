## Source layout

Each `src/binaries/<binary>/` is a self-contained sub-project that holds every file for that binary, headers included. Library headers go in `src/common/` (enforced by the `header-layout` meson test).

## Agent skills

### Issue tracker

Issues are tracked in GitHub Issues for `brandon515/Velora` via the `gh` CLI. See `docs/agents/issue-tracker.md`.

Filing a bug: one issue per failing test, with Expected and Actual sections, and the test marked `RUN_TEST_KNOWN_BUG`. See `docs/agents/issue-tracker.md#filing-a-bug`.

### Triage labels

Uses the five default labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `GLOSSARY.md` and `docs/adr/` at the repo root. See `docs/agents/domain.md`.
