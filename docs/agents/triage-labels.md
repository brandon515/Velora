# Triage Labels

The skills speak in terms of five canonical triage roles. This file maps those roles to the actual label strings used in this repo's issue tracker.

| Label in mattpocock/skills | Label in our tracker | Meaning                                  |
| -------------------------- | -------------------- | ---------------------------------------- |
| `needs-triage`             | `needs-triage`       | Maintainer needs to evaluate this issue  |
| `needs-info`               | `needs-info`         | Waiting on reporter for more information |
| `ready-for-agent`          | `ready-for-agent`    | Fully specified, ready for an AFK agent  |
| `ready-for-human`          | `ready-for-human`    | Requires human implementation            |
| `wontfix`                  | `wontfix`            | Will not be actioned                     |

When a skill mentions a role (e.g. "apply the AFK-ready triage label"), use the corresponding label string from this table.

Edit the "Label in our tracker" column to match whatever vocabulary you actually use.

## Severity tiers

Every bug issue also gets exactly one tier label. The tier says how bad the bug is when it happens. It doesn't say how likely the bug is or how hard it is to fix. If a bug could fit more than one tier, use the most severe one.

| Label    | Meaning | Examples |
| -------- | ------- | -------- |
| `tier-1` | **Crashes or memory corruption in normal use.** Stack or heap overflows, use-after-free, double free, out-of-bounds reads or writes, undefined behaviour that sanitizers report, hangs or infinite loops, and anything that makes the engine fail to start or shut down cleanly. | `darray` stack overflow on large arrays, event system use-after-free on restart, glTF thread-array overflow |
| `tier-2` | **Wrong results or files that won't load, without a crash.** Incorrect output, valid input rejected, behaviour that doesn't match the API or the header, and build or link failures in a configuration we support. | transposed rotation matrix, OpenGL depth range on a Vulkan renderer, `vstrtoint` never computing a value |
| `tier-3` | **Leaks and resource cleanup (lower priority).** Memory or OS handles that are never released, cleanup paths that are missing or wrong, stubs that waste resources (such as busy-spinning), and checks that are dead but harmless today. | vjson never freeing its input file, X display never closed, empty `platform_sleep` |

Fix order follows the tiers: `tier-1` before `tier-2` before `tier-3`, unless a lower-tier bug is blocking a higher-tier fix or hiding it. For example, the Linux `platform_get_absolute_time` returning 0 hides a `pump_events` bug.

When you file a new bug, follow **Filing a bug** in `issue-tracker.md` (one issue per failing test, with expected and actual results), and give it `bug`, `needs-triage` and a tier label. If you can't tell the tier yet, leave the tier label off and let triage decide.
