# Asset loaders live in velora_assets, not velora_core

The JSON, glTF and image loaders (`vjson`, `vgltf`, `vimage`, and the bundled `stb_image`) are built as their own shared library, `velora_assets`, on top of `velora_core`. Their public headers are in `src/common/assets/` and use `VASSETS_API`, following ADR 0001. The dependency only goes one way: velora_ecs, velora_render, vulkan_render and the game link velora_assets, velora_assets links velora_core, and core never includes an `assets/` header. `stb_image.h` is in `src/common/assets/` with the other headers (every header goes in `src/common/`), but only velora_assets includes it, and its functions aren't marked `VASSETS_API`, so they aren't exported.

Before the split, the loaders were about half of core's code and held 13 of its 33 open bugs. They do a different job from the rest of core: they parse untrusted files, while the rest of core is the runtime foundation (memory, logging, platform, events, containers, strings, file I/O, math). Keeping them separate lets the loaders change, and later be replaced, without rebuilding everything that depends on core. It also keeps stb_image out of the foundation.

## Considered Options

- **Leave everything in core.** Simplest, but core keeps growing with every new asset format, and every consumer of core pulls in the parsers.
- **Split the foundation further (memory, logger, platform, events as separate libraries).** Rejected because those modules include each other: platform uses logger, events and memory; logger and memory call back into platform; events need platform for time. Separate shared libraries would depend on each other in a cycle.
- **Move `vmath` out too.** It has no dependencies except `<math.h>`, so it could go anywhere, but at ~550 lines it isn't worth its own DLL yet. Because nothing ties it down, moving it later is cheap.
- **Move `vfile` with the loaders.** Rejected: it's general file I/O built on `vstring` and memory, so it belongs in core.

## Consequences

- New asset formats go in `src/libraries/assets/` (sources) and `src/common/assets/` (headers) with `VASSETS_API`, not in core.
- Any target that includes an `assets/` header must link `out_assets_lib`. `velora_tests` compiles the assets sources directly and defines `VELORA_ASSETS_BUILD`, as it does for core and ecs.
- `ecs/vskin.h` includes `assets/vgltf.h`, so ecs depends on assets. That's a layering smell tracked in #61. Once it's fixed, ecs should depend only on core.
