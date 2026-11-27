# Velora

A small game engine written in C, with a Vulkan renderer. It runs on Linux (X11) and Windows (Win32).

## Layout

All library headers, public and private, live in `src/common/`, with one directory per library. Library implementations (`.c` files only) live in `src/libraries/`, with one directory per library.

| Library | Kind | What it holds |
| ------- | ---- | ------------- |
| `velora_core` | shared | Memory (`vmemory`), logging, asserts, platform layer (Linux/Win32), events, containers (`darray`, `map`), strings, file I/O, math |
| `velora_assets` | shared | JSON, glTF and image loaders (`vjson`, `vgltf`, `vimage`, bundled `stb_image`) |
| `velora_ecs` | shared | Entity component system and its components (transform, camera, renderable, skin) |
| `vulkan_render` | shared | The Vulkan render backend, its shaders (`shaders/*.glsl`) and Vulkan Memory Allocator |
| `velora_render` | shared | The render frontend: meshes and materials, calling into the backend |
| `velora_entry` | static | `main()` and the main loop. It calls the application's `velora_init`, `velora_frame` and `velora_exit` (`entry/entry.h`) |

Executables are in `src/binaries/`. Each directory there is a self-contained sub-project with all of its own files, headers included:

- `engine` (`game/`): the game application. Right now it loads `Cube.gltf` (from `assets/`) and renders it.
- `lib_test` (`lib_test/`): a scratch program that links every library.
- `velora_tests` (`testing/`): the unit tests.

Each shared library exports only the functions marked with its own API macro (`VCORE_API`, `VASSETS_API`, `VECS_API`, `VRENDER_API`, `VRENDER_BACKEND_API`). See [ADR 0001](docs/adr/0001-shared-libraries-with-per-library-api-macros.md) and [ADR 0002](docs/adr/0002-asset-loaders-in-their-own-library.md) for why the code is split this way.

## Building

You need Meson, Ninja, clang (CI uses clang on both platforms), the Vulkan SDK (1.4; on Linux the LunarG SDK, because Ubuntu 24.04 only packages 1.3), Vulkan Memory Allocator and `glslc`. On Linux you also need `libx11-dev`. The Meson wraps in `subprojects/` can supply Vulkan headers, VMA and libxkbcommon.

```sh
meson setup builddir
meson compile -C builddir
meson install -C builddir   # copies engine, lib_test, shaders and the libraries into bin/
```

The install step puts the executables, compiled shaders and the files in `assets/` in `bin/`, and the shared libraries in `bin/plugins/`. Run the game from inside `bin/`, because it loads shaders and models from the working directory.

`assets/` holds the game's default cube and the renderer's fallback texture (`DefaultTexture.png`). `tools/make_cube_assets.py` generates all of them, so edit the script and rerun it rather than editing the files.

## Tests

```sh
meson compile -C builddir velora_tests
meson test -C builddir                                   # every suite
meson test -C builddir vgltf                             # one suite
./builddir/src/binaries/testing/velora_tests vgltf       # one suite, printing each check
./builddir/src/binaries/testing/velora_tests --failures  # only failing tests and their output, then a summary
```

The tests compile the core, assets and ECS sources and the engine's main loop directly, so they need no GPU or window. On Linux each test case runs in its own forked process, so a crash only fails that test. Memory errors are only caught reliably in a sanitizer build:

```sh
meson setup build-asan -Dbuildtype=debug -Db_sanitize=address,undefined -Db_lundef=false
```

A test for an open bug is marked `RUN_TEST_KNOWN_BUG(test, issue)`, so it shows as XFAIL and the suite still passes. When the bug is fixed the test shows as XPASS and fails the suite, which is the signal to switch it to `RUN_TEST` and close the issue. See `src/binaries/testing/test_framework.h`.

CI (`.github/workflows/ci.yml`) runs the tests on Linux (sanitizer and release builds) and on Windows, and checks that every known-bug test names an open issue. It also does a full Linux build, validates the shaders, and runs clang-tidy and clang-format as advisory checks.

## Docs

- [`docs/adr/`](docs/adr/): architecture decisions.
- [`docs/agents/`](docs/agents/): how issues, triage labels and severity tiers work in this repo.

## Bugs

Bugs are tracked only in [GitHub Issues](https://github.com/brandon515/Velora/issues), not in files in the repo. Each bug issue covers one failing test, and says what the test expects and what happens now. See [`docs/agents/issue-tracker.md`](docs/agents/issue-tracker.md#filing-a-bug).
