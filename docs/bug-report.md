# Velora bug report

## Context
This started as a review of the whole codebase for potential bugs, skipping unfinished features such as the buddy allocator. It was last updated on 2026-11-23, when the unit tests (`velora_tests`, `src/binaries/testing/`) were expanded so that a failing test reproduces each bug the tests can reach.

**Status key**
- **Open:** still present in the code.
- **Fixed:** fixed. A passing regression test keeps it fixed where one exists.
- **Partly fixed:** some of the problem is fixed. The entry says what's left.

Items marked ✔ were checked against the code by hand. The rest come from reading the code and, where a test is listed, the failing test confirms them.

**Tests:** `suite / test_name` points at a test in `src/binaries/testing/<suite>_test.c`. Run one suite with `meson test -C <builddir> <suite>`, or `velora_tests <suite>` to see each check. On Linux each test case runs in its own forked process, so a test that crashes or hangs on a bug is counted as one failure and the rest of the suite still runs. Tests that crash are only reliable under the ASan/UBSan build (`-Db_sanitize=address,undefined`). The release build may let a memory error pass silently.

Bugs that need a GPU, a window or `input.c` have no test. Their entries say so.

---

Tiers are defined in `docs/agents/triage-labels.md` (labels `tier-1`, `tier-2`, `tier-3`). Every open item is filed as a GitHub issue with its tier label.

## Tier 1: Crashes or memory corruption in normal use

### darray (`src/libraries/core/container/darray.c`), which sits under events, input, map and ECS
1. ✔ **Fixed.** **`darray_get_pointer`** computed `index*stride*index`. Test: `darray / test_get_pointer_every_index`.
2. ✔ **Fixed.** **`darray_get_data`** had its bounds check backwards and no `return TRUE`. Test: `darray / test_get_out_of_range`.
3. ✔ **Fixed.** **`darray_insert`** wrote `stride` bytes past the end of its stack buffer. It now uses `vmove_memory`. Tests: `darray / test_insert_front_small_array`, `ecs / test_attach_out_of_order_ids`.
4. ✔ **Fixed.** **`darray_insert`** rejected `index == length`, which broke replacing the last key in `map_set_item`. Tests: `darray / test_insert_at_length_appends`, `map / test_replace_last_key`.
5. **Open.** **`darray_reallocate`, `darray_remove`, `darray_delete` and `darray_drain`** still copy through a stack buffer of size `cap*stride`, so a large array overflows the stack (8 MB on Linux, 1 MB on Windows). Use `vreallocate` and `vmove_memory` instead. Test: `darray / test_large_array_does_not_overflow_stack`, which hits a stack overflow in `darray_reallocate`. This also makes JSON #22 crash instead of only running out of memory.

### ECS (`src/libraries/ecs/vecs.c`)
6. ✔ **Fixed.** **`delete_component`** passed the element buffer to `darray_delete`. Test: `ecs / test_delete_component`.
7. ✔ **Fixed.** **`delete_entity`** used `i` where it meant `j`. Test: `ecs / test_delete_entity`.

### Input (`src/libraries/core/input.c`)
8. **Open, can't be tested.** **`register_input_mapping`** pushes a stack `game_input` struct into an array whose element size is `sizeof(game_input*)`. Readers then treat the stored name pointer as a `game_input*`. It also never writes `out_id`, and `out_id` is `u32*` while the ID is `u64`.
9. **Open, can't be tested.** **`unbind_input_action`** doesn't deregister the binding's listener or free the binding. **`shutdown_input_sytem`** leaks every binding. **`action_data`** is never set, so every binding matches button or axis 0.

   `input.c` is not in the tree right now, so #8 and #9 have no tests. Check them again when it comes back.

### Renderer (`src/libraries/vulkan_render/velora_vulkan_render.c`). Needs a GPU, so none of these are tested.
10. ✔ **Fixed.** **`obtain_physical_device`** incremented `deviceCount` instead of `i`.
11. ✔ **Fixed.** **Uniform writes were indexed twice.** The code now writes through the pointer from `get_UBO_ptr`.
12. ✔ **Fixed.** **`vertexOffset`** was a byte offset passed to `vkCmdDrawIndexed`, which counts vertices. It is now divided by `sizeof(vertex)`, and indices live in their own region after `VERTEX_BUFFER_SIZE`.
13. ✔ **Open.** **`create_swapchain` (~851–856)** points `pQueueFamilyIndices` at `indices[2]`, which goes out of scope before `vkCreateSwapchainKHR` reads it. This affects GPUs whose present and graphics queue families differ.
14. **Open.** **Swapchain recreation:**
    - The `OUT_OF_DATE` loop in `render_frame` (~2215) has no exit condition and no minimize check, so it can recreate with a 0×0 extent (invalid on Win32).
    - If `recreate_swapchain` fails there, `render_frame` returns before the submit. The fence was already reset in `render_preframe`, so the next `vkWaitForFences(U64_MAX)` hangs forever, and shutdown double-frees the half-destroyed swapchain.
    - `render_preframe` (~2180) only treats the window as minimized when **both** dimensions are 0.
    - `renderFinished[]` is sized once by the swapchain image count and never resized when recreation changes that count.
15. **Open.** **`update_descriptor_sets` (~1765)** declares `VkDescriptorBufferInfo skinBufferInfo[SKIN_BUFFER_COUNT]` on the stack. That's 65536 entries, about 1.5 MB, which overflows the 1 MB Windows stack. That 65536-entry uniform-buffer binding is also far above typical device limits, and the dynamic-indexing features it needs are never enabled.

### glTF (`src/libraries/core/utils/vgltf.c`)
16. ✔ **Open.** **`thrd_t threadIds[512]`:** the importer creates one thread per glTF object with no limit, so a model with more than 512 objects writes past the stack array. Test: `vgltf / test_more_than_512_objects` (crashes).
17. ✔ **Open.** **Accessor `byteOffset` is ignored** in `gltf_get_stream`, so every interleaved or shared bufferView returns the wrong data. Tests: `vgltf / test_accessor_byte_offset`, `vgltf / test_interleaved_attributes`.
18. **Open.** **No size checks:**
    - `extract_gltf_buffer` copies `byteLength` bytes without comparing it to the .bin size. Test: `vgltf / test_bin_shorter_than_byte_length` (heap-buffer-overflow).
    - `gltf_get_stream` checks reads against the buffer instead of the buffer view. Test: `vgltf / test_accessor_past_view_rejected`.

### Math and transforms
19. ✔ **Open.** **`rotation_matrix` (`vmath.c:180-188`)** is transposed for the column-major layout (`mat[1]` should be `2(xy+zw)`), so every quaternion rotates the opposite way. Tests: `vmath / test_rotation_matrix_z_90`, `test_rotation_matrix_x_90`, `test_model_matrix_order`.
20. **Open.** **`rotation_to_quat` (`vmath.c:481,487,493`):** the three fallback branches use `+` where `−` is needed for `w`. The original report said this breaks rotations over ~120°. In fact the fallback only runs when `1 + trace ≤ 0`, which means rotations at or very close to 180°. `rotation_to_quat` also uses the same transposed layout as #19, so fix the two together. Test: `vmath / test_decompose_180_degree_rotation`. `test_decompose_round_trip` passes today and should keep passing once both are fixed.
21. ✔ **Open.** **`transform_get_model_matrix` (`vtransform.c:82`):** `matrix4_multiply(parent, local)` gives local·parent, given this library's argument order (`matrix4_multiply(a, b)` applies a first, then b). Child transforms are combined in the wrong order. Test: `ecs / test_model_matrix_with_parent`. `vmath / test_matrix4_multiply_convention` pins the argument order so the fix doesn't flip it by accident.

### JSON (`src/libraries/core/utils/vjson.c`)
22. **Open.**
    - **`null` in an array (~196):** the index never advances, so the parser loops forever. It hits the darray stack overflow (#5) within seconds. Test: `vjson / test_null_in_array` (crashes).
    - **`null` in an object (~259):** the index jumps by a stale `amountToMove` and import fails. Test: `vjson / test_null_in_object`.
23. **Open.** **No end-of-buffer check:** the input has no NUL terminator (`vfile.c`), and the whitespace, `:` and string scanners have no end check. Any truncated or malformed file reads past the buffer. Test: `vjson / test_truncated_input_fails_cleanly` (heap-buffer-overflow in `process_json_string`).

### Entry (`src/libraries/entry/entry.c`). Needs a window, so not tested.
24. **Open.**
    - **Closing the window doesn't stop the main loop (~70):** the loop only checks `velora_frame`'s return value and never looks at `is_running`, which `close_event_handler` sets.
    - **`velora_exit` (~80)** is passed `engine_state` instead of `engine_state->app_data`.

---

## Tier 2: Wrong results or files that won't load (no crash)
- **Open. `pump_events` (`event.c:99-106`)** drains `i` events instead of `i+1` when it runs out of time, so one event fires twice and its data is freed twice. Today this is hidden only because `platform_get_absolute_time` returns 0 on Linux. Tests: `event / test_pump_over_time_limit_does_not_repeat_events`, `test_pump_over_time_limit_does_not_double_free` (use-after-free).
- **Open. `pump_events`/`fire_event`** keep pointers into darrays that a handler can reallocate. Tests: `event / test_event_queued_by_handler_is_delivered_later`, `test_listener_registered_by_handler` (both use-after-free).
- **Open. `fire_event` leaks event data** when no listeners are registered at all. Test: `event / test_fire_with_no_listeners_frees_data`. It also frees data it may not own.
- **Open. `vsubstr` (`vstring.c:34`)** uses `>=` where it should use `>`, so a substring that ends exactly at the end of the string returns NULL. That also makes `get_file_path("models/")` return NULL. Tests: `vstring / test_substr_to_end_of_string`, `vfile / test_get_file_path_of_directory`.
- **Open. `vstrtoint`** never computes a value. It indexes `str[index]` instead of `str[i]`, and `place` never grows. Test: `vstring / test_strtoint`.
- **Open. The present-error log (~2331)** prints `imageErr` instead of `err`. Present `OUT_OF_DATE` and `SUBOPTIMAL` results never trigger a recreate. Not tested.
- **Open. The `ubo` struct** uses `u64` for `textureIndex` and `skinOffset`, but the shader uses `uint`, so the shader's `skinOffset` is always 0. Not tested.
- **Open. JSON number parsing:**
    - `load_json_float_array` rejects integers (`[0,0,0,1]`), so glTF transforms and colours silently fall back to defaults. Tests: `vjson / test_float_array_accepts_integers`, `vgltf / test_integer_node_values`.
    - `1e+20` doesn't parse. Test: `vjson / test_exponent_numbers`.
    - `[ ]` with a space inside doesn't parse. Test: `vjson / test_empty_array_with_space`.
- **Open. glTF booleans** (`normalized`, `doubleSided`) are read as integers, so they always come out FALSE. Test: `vgltf / test_accessor_normalized_flag` (only `normalized` is tested).
- **Open. Non-float vector accessors** such as JOINTS_0 (unsigned byte VEC4) are rejected. Test: `vgltf / test_non_float_vector_accessor`.
- **Open. The projection matrices** (perspective and orthographic) map depth to OpenGL's [-1,1], but the renderer is Vulkan, which expects [0,1]. Tests: `vmath / test_perspective_depth_range`, `test_orthographic_depth_range`.
- **Open. The depth buffer** is shared between frames in flight without a write-after-write dependency. Not tested.
- **Open. `VASSERT_MSG`** takes 1 argument when assertions are disabled and 2 when enabled, so builds with assertions disabled fail. It can't be tested while `asserts.h` turns on `VASSERTIONS_ENABLED` unconditionally.
- **Fixed. `VEL_CHECK`** didn't parenthesize `expr` (commit a8265d3).
- **Open. `logger.c:36`:** `sprintf` of the level prefix plus a message of up to 31999 characters into a 32000-byte buffer overflows `out_message`. Test: `logger / test_message_at_buffer_limit`. It fails under ASan; the release build doesn't detect it.

## Tier 3: Leaks and resource cleanup (lower priority)
- **Open. vjson:** the input file is never freed, and value names are never freed. When a parse fails, `VEL_CHECK` also leaks the partly built darray. Test: `vjson / test_import_then_free_returns_all_memory`. Most other JSON and glTF tests skip the leak check until this is fixed; see `RUN_TEST_IGNORE_LEAKS`.
- **Open. vgltf:** `uriPath` is never freed. Test: `vgltf / test_import_then_free_returns_all_memory`.
- **Open. `vfile.c`:**
    - The buffer leaks on a short read. Not tested.
    - `get_file_path` returns the literal `"./"` for a bare file name, so callers can't free its result safely. The header says it returns NULL. Test: `vfile / test_get_file_path_of_bare_name_is_freeable`.
- **Open. Renderer:** swapchain support arrays leak per candidate GPU, staging buffers leak on error paths, and the default texture's pixels are never freed. Not tested.
- **Open. `vmesh.c`:** only the last primitive is kept and the earlier ones leak. The per-vertex arrays are on the stack. Not tested.
- **Open. Linux platform:**
    - The X display, input method and input context are never closed. Not tested.
    - `platform_sleep` is empty, so the loop busy-spins while minimized. Test: `platform / test_sleep_waits`.
    - `platform_get_absolute_time` returns 0, which hides the `pump_events` bug and breaks any timing. Test: `platform / test_absolute_time_advances`.

---

## New bugs found while writing the tests (2026-11-23)
These were not in the original review. None are fixed.

25. **Tier 1, event system restart (`event.c:29-32`):** `shutdown_event_system` frees `event_queue` and `event_listeners` but doesn't set them back to NULL, so `initiate_event_system` afterwards keeps the freed arrays and the next register or queue writes to freed memory. Test: `event / test_restart_after_shutdown` (use-after-free).
26. **Tier 1, `delete_entity` bounds (`vecs.c:161`):** it reads `componentLists.entities[entityID]` before checking `entityID < VELORA_MAX_OBJECTS`, so an out-of-range ID reads past the global array. `attach_component` does check. Test: `ecs / test_delete_entity_out_of_range` (global-buffer-overflow).
27. **Tier 2, `matrix_vec4_multiply` (and the vec2/vec3 versions, `vmath.c` `matrix_vec_multiply`):** these index the matrix as row-major while `mat4` is column-major, so they compute Mᵀ·v. Applying `translation_matrix` through `matrix_vec4_multiply` doesn't move the point. Nothing calls these yet. Test: `vmath / test_matrix_vec4_multiply`.
28. **Tier 2, `vinttostr(INT64_MIN)` (`vstring.c:109`):** `scratchValue *= -1` overflows a signed integer, which is undefined behaviour. Test: `vstring / test_inttostr_int64_min` (UBSan).
29. **Tier 3, `load_json_string_array` failure path (`vjson.c:530`):** the cleanup loop frees `(*value)[i]` instead of `(*value)[j]`. That frees the same (NULL) slot over and over and leaks every string already copied. vmemory's byte count still balances because the sizes come from `[j]`, so no vmemory-based test can catch it. Not tested.
30. **Tier 2, glTF header/source mismatch:** `vgltf.h:332` declares `gltf_accessor_get_element_size_type`, but `vgltf.c` defines `gltf_accessor_get_element_size`. Code that calls the declared name fails to link. Not tested, because a test calling it would break the build.
31. **Tier 3, Win32 timer depends on the window:** `platform_get_absolute_time` (`platform_win32.c:190`) multiplies by `clock_frequency`, which is only set in `platform_startup`. Before a window exists, the timer returns 0. Not tested; `test_absolute_time_advances` runs on Linux only for this reason.
32. **Tier 3, dead input checks in the JSON number parsers:** `process_json_integer` tests `c < '0' && c > '9'` and `process_json_float` tests `c <= '0' && c >= '9'`. Neither condition can ever be true, so neither function rejects bad input itself. `identify_json_value` currently filters the input first, so this is harmless today. Not tested.

---

## Proposed fix order
1. darray #5. It also turns JSON #22's crash into a plain infinite loop, which still needs its own fix.
2. Event system: #25, then the Tier 2 `pump_events`/`fire_event` items.
3. Math #19 and #20 together, then #21 (the multiply-convention test should keep passing). Then #27 and the projection depth range.
4. JSON #22, #23 and the Tier 2 number parsing, then glTF #16–18 and the Tier 2 glTF items.
5. ECS #26, strings (`vsubstr`, `vstrtoint`, #28), logger overflow, `get_file_path`.
6. Renderer #13–15 and entry #24 (need manual testing, see below).
7. Tier 3 leaks. Switch the matching tests from `RUN_TEST_IGNORE_LEAKS` back to `RUN_TEST` as each leak is fixed.

## Verification
- Run `meson test -C <builddir>` under both CI configurations (`-Dbuildtype=debug -Db_sanitize=address,undefined -Db_lundef=false`, and `-Dbuildtype=release`). Each fix should turn its listed test green without turning any other test red.
- For the renderer and entry items, run the game with Vulkan validation layers on: register two or more meshes and objects, resize, minimize and restore, then close the window and confirm it exits cleanly.
- For input #8–9, add tests to `src/binaries/testing/` once `input.c` is back in the tree.
