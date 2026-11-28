# Platform data crosses library boundaries only through accessors

Other libraries never see the platform layer's per-OS state. `core/platform/platform.h` includes no OS headers. `platform_state` there is a fixed public struct that holds one `void *internal_state` handle. The real struct (`linux_state`, and its Win32 equivalent) is defined only in `platform_linux.c` / `platform_win32.c`, which allocate it in `platform_startup` and free it in `platform_shutdown`. Any other library that needs platform data, vulkan_render included, gets it through a `VCORE_API` function in `platform.h`:

- **Data that means the same on every OS** (window size today; mouse position or DPI later) gets a typed accessor such as `platform_get_window_size`.
- **OS handles** that a graphics API needs to create a surface come from `platform_get_native_window`. It fills a `platform_native_window`, which holds a `kind` tag and a union of plain types (`void*`, `u64`). The caller checks `kind` and casts the values back to `Display*`/`Window` or `HINSTANCE`/`HWND` in its own `.c` file.

We chose this because `logger.h` includes `platform.h`, so OS headers in `platform.h` reached every file that logs (#60). It also keeps the layout of the per-OS struct private to velora_core. Under ADR 0001, core and vulkan_render are separate shared libraries that should rebuild on their own. If vulkan_render read the struct's fields directly, adding a field to core would break it silently until both were rebuilt. Fields the renderer has no business reading (`xim`, `xic`, `wm_delete_window`) also stay hidden.

## Considered Options

- **Share the full struct through an "internal" header** included only by the platform sources and vulkan_render. Rejected: every header lives in `src/common/` (enforced by the `header-layout` test), so nothing keeps other targets from including it. It would also make the struct layout part of the ABI between two shared libraries.
- **A platform-provided surface hook** (`platform_create_vulkan_surface`). Rejected because core would then depend on Vulkan. Under this rule a second render backend needs no change to core.
- **A forward-declared opaque `platform_state` returned by pointer from `platform_startup`.** This is just as private, but every holder (`entry.h`, `entry.c`, `lib_test/main.c`) would have to change. The wrapper around a `void*` handle keeps them holding `platform_state` by value.
- **One accessor per native field** (`platform_get_native_display`, `platform_get_native_window`, ...). Rejected: the list grows with every OS. The tagged union groups the handles by window system, so a new backend such as Wayland is one new `kind` and one new union member.

## Consequences

- A library that needs new platform data asks for a new accessor in `platform.h` (marked `VCORE_API`, per ADR 0001). It never includes a platform source's types.
- Supporting a new window system means adding a `platform_window_kind` value and a union member. Every consumer must handle a `kind` it doesn't support by failing with an error, as `create_window_surface` does.
- OS headers can still appear in a consumer's `.c` file when its own API needs them (for example, `vulkan_xlib.h` needs `X11/Xlib.h`). They never go in a header under `src/common/`.
- Calling an accessor before `platform_startup` or after `platform_shutdown` returns `FALSE`, because `internal_state` is `NULL` then.
