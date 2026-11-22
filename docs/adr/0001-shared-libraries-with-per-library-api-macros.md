# Engine libraries are shared libraries with one API macro per library

velora_core, velora_ecs, velora_render and vulkan_render are built with `shared_library()` and hidden symbol visibility. Each one exports only the functions its own macro marks: `VCORE_API`, `VECS_API`, `VRENDER_API` or `VRENDER_BACKEND_API`, defined in `src/common/*/*_api.h`. A macro expands to `VEXPORT` when Meson passes that library's `-DVELORA_<LIB>_BUILD`, and to `VIMPORT` in every other target. We chose shared libraries so libraries can be rebuilt and relinked on their own and swapped later (for example a second render backend or hot-reloaded game code). A single macro can't do this job because one source file includes headers from several libraries. On Windows, it has to export its own declarations and import everyone else's.

## Considered Options

- **Static libraries.** Simpler, portable, and allow link-time optimization across the whole engine. Rejected because they rule out swapping or reloading parts of the engine at runtime.
- **Using `VEXPORT` on every declaration.** On Windows this happens to work for functions, through import-library stubs. It breaks for any global variable shared between libraries, and it can't say which library owns a symbol.

## Consequences

- Every new public function in a library header needs that library's macro. Without it, Linux linking fails with an undefined reference, and Windows silently leaves the function out of the DLL.
- The render backend interface is declared in `render/velora_render.h` but implemented by vulkan_render, so those declarations use `VRENDER_BACKEND_API`, not `VRENDER_API`.
- `velora_entry` stays a static library because it defines `main()` and calls application callbacks that live in the executable.
- `velora_tests` compiles core's sources directly, so it defines `VELORA_CORE_BUILD` itself.
