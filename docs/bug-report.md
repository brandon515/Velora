# Velora bug report

## Context
You asked for a review of the whole codebase for potential bugs, skipping unfinished features such as the buddy allocator. Three read-only searches covered (1) core containers, events and input, (2) the Vulkan renderer and platform layer, and (3) JSON, glTF, math and ECS. I checked the items marked ✔ against the code myself. The rest come from reading the code but I haven't verified them. This file is the report; nothing has been changed. If you approve, I'll fix the bugs in Tier 1 and Tier 2, a few at a time, in the order below.

---

## Tier 1: Crashes or memory corruption in normal use

### darray (`src/libraries/core/container/darray.c`), which sits under events, input, map and ECS
1. ✔ **`darray_get_pointer:133`** computes `index*stride*index`, so the offset grows with the square of the index. Index 2 and above read the wrong element or past the end of the array. Every iterator uses this function (ECS, camera, renderer object loop).
2. ✔ **`darray_get_data:137`** has its bounds check backwards (`length >= index` → should be `index >= length`) and has no `return TRUE`.
3. ✔ **`darray_insert:59-68`** copies `cap*stride - mem_location` bytes starting at `buffer+mem_location+stride`, which writes `stride` bytes past the end of the stack buffer. `attach_component` reaches this whenever a component is added for an entity whose ID is lower than one already in the list.
4. ✔ **`darray_insert`** rejects `index == length`. `map_set_item` (`map.c:38-39`) deletes the entry and then re-inserts it at the same index, so replacing the last key sets `map->data = NULL` and leaks the old array.
5. **All functions** copy through a stack buffer of size `cap*stride`, so a large array overflows the stack. Use `vreallocate` and `memmove` instead.

### ECS (`src/libraries/ecs/vecs.c`)
6. ✔ **`delete_component:138`** passes `compTypeList.data->data` (the element buffer) to `darray_delete` instead of the `darray*`.
7. ✔ **`delete_entity:168-170`** uses `i` where it should use `j`, so it frees and deletes the wrong components.

### Input (`src/libraries/core/input.c`)
8. ✔ **`register_input_mapping`** pushes a stack `game_input` struct into an array whose element size is `sizeof(game_input*)`. Readers then treat the stored name pointer as a `game_input*`, which crashes or writes into a string literal. It also never writes `out_id`, and `out_id` is `u32*` while the ID is `u64`.
9. **`unbind_input_action`** doesn't deregister the binding's listener or free the binding. **`shutdown_input_sytem`** leaks every binding. **`action_data`** is never set, so every binding matches button or axis 0.

### Renderer (`src/libraries/vulkan_render/velora_vulkan_render.c`)
10. ✔ **`obtain_physical_device:718`** increments `deviceCount` instead of `i`. If the first GPU isn't suitable, startup hangs forever and leaks memory.
11. ✔ **Uniform writes are indexed twice (2238-2246).** `get_UBO_ptr(..., frame*N+k)` already points at slot k, and the code then indexes `[k]` again with the unaligned `sizeof(ubo)`. Every object after the first gets the wrong transform, and near the end of the buffer the write goes past the mapped memory.
12. ✔ **`vertexOffset` (1866, 2265)** is a byte offset passed as `vkCmdDrawIndexed`'s vertexOffset, which counts vertices. Vertices and indices are also interleaved in a single buffer. Every mesh after the first draws garbage.
13. ✔ **`create_swapchain:849`** points `pQueueFamilyIndices` at `indices[2]`, which goes out of scope before `vkCreateSwapchainKHR` reads it. This affects GPUs whose present and graphics queue families differ.
14. **Swapchain-recreate loop (2186-2197, your last commit):**
    - It has no exit condition and no minimize check, so it can recreate with a 0×0 extent (invalid on Win32).
    - If `recreate_swapchain` fails, `render_frame` returns before the submit. The fence was already reset in preframe, so the next `vkWaitForFences(U64_MAX)` hangs forever, and shutdown double-frees the half-destroyed swapchain.
    - `render_preframe:2152` only treats the window as minimized when **both** dimensions are 0.
    - `renderFinished[]` is sized once by the swapchain image count and never resized when recreation changes that count.
15. **`update_descriptor_sets:1764`** declares a stack array of about 1.5 MB (`SKIN_BUFFER_COUNT`=65536), which overflows the 1 MB Windows stack. That 65536-entry uniform-buffer binding is also far above typical device limits, and the dynamic-indexing features it needs are never enabled.

### glTF (`src/libraries/core/utils/vgltf.c`)
16. ✔ **`thrd_t threadIds[512]` (714):** the importer creates one thread per glTF object with no limit, so a model with more than 512 objects writes past the stack array.
17. ✔ **Accessor `byteOffset` is ignored (1287),** so every interleaved or shared bufferView returns the wrong data.
18. **No size check against the file (53, 1293):** the buffer copy trusts `byteLength` against the .bin size, and the read check uses the buffer bounds instead of the view's.

### Math and transforms
19. ✔ **`rotation_matrix` (`vmath.c:180-188`)** is transposed for the column-major layout (`mat[1]` should be `2(xy+zw)`), so every quaternion rotates the opposite way. To confirm: rotate +90° about Z, then check where X ends up.
20. **`rotation_to_quat` (`vmath.c:481,487,493`):** the three fallback branches use `+` where `−` is needed for `w`, so it's wrong for rotations over ~120°.
21. ✔ **`transform_get_model_matrix` (`vtransform.c:82`):** `matrix4_multiply(parent, local)` gives local·parent, given this library's argument order (which `model_matrix` and the MVP code follow). Child transforms are combined in the wrong order.

### JSON (`src/libraries/core/utils/vjson.c`)
22. **`null` values (196, 259):** a `null` element never advances the index, so an array containing `null` loops forever until memory runs out. In objects it jumps by the wrong amount.
23. **No end-of-buffer check:** the input has no NUL terminator (`vfile.c`), and the whitespace, `:` and string scanners have no end check. Any truncated or malformed file reads past the buffer.

### Entry (uncommitted work in progress)
24. **Closing the window doesn't stop the main loop (`entry.c:73`),** because it never checks `is_running`. **`velora_exit` (`entry.c:83`)** is passed `engine_state` instead of `app_data`.

---

## Tier 2: Wrong results or files that won't load (no crash)
- **`pump_events` (`event.c:99-106`)** drains `i` events instead of `i+1`, so one event fires twice and its data is freed twice. This is hidden today only because `platform_get_absolute_time` returns 0.
- **`pump_events`/`fire_event`** keep pointers into darrays that a handler can reallocate. `fire_event` also leaks event data when there are no listeners, and frees data it may not own.
- **`vsubstr` (`vstring.c:34`)** uses `>=` where it should use `>`, so a substring that ends exactly at the end of the string returns NULL. `vstrtoint` never computes a value, because it indexes `str[index]` instead of `str[i]`.
- **The present-error log (2303)** prints `imageErr` instead of `err`. Present `OUT_OF_DATE` and `SUBOPTIMAL` results never trigger a recreate.
- **The `ubo` struct** uses `u64` for `textureIndex` and `skinOffset`, but the shader uses `uint`, so the shader's `skinOffset` is always 0.
- **The JSON float loader** rejects integers (`[0,0,0,1]`), so transforms and colours silently fall back to defaults. `1e+20` doesn't parse, and neither does `[ ]` with a space inside.
- **glTF booleans** (`normalized`, `doubleSided`) are read as integers, so they always come out FALSE. Non-float vector accessors (JOINTS_0) are rejected.
- **The projection matrix** maps depth to the OpenGL range [-1,1], but the renderer is Vulkan, which expects [0,1].
- **The depth buffer** is shared between frames in flight without a write-after-write dependency.
- **`VASSERT_MSG`** takes 1 argument when assertions are disabled and 2 when enabled, so builds with assertions disabled fail. `VEL_CHECK` doesn't parenthesize `expr`.
- **`logger.c:36`:** `sprintf` can overflow `out_message`.

## Tier 3: Leaks and resource cleanup (lower priority)
- **vjson:** the input file is never freed, and value names are never freed.
- **vgltf:** `uriPath` is never freed.
- **`vfile.c`:** the buffer leaks on a short read. `get_file_path` sometimes returns the literal `"./"`, so callers can't free its result safely.
- **Renderer:** swapchain support arrays leak per candidate GPU, staging buffers leak on error paths, and the default texture's pixels are never freed.
- **`vmesh.c`:** only the last primitive is kept and the earlier ones leak. The per-vertex arrays are on the stack.
- **Linux platform:** the X display, input method and input context are never closed. `platform_sleep` is empty, so the loop busy-spins while minimized.

---

## Proposed fix order (if approved)
1. darray #1–5 (it fixes the most downstream problems), then ECS #6–7 and map #4.
2. Input #8–9.
3. Renderer #10–14, then #15.
4. Math #19–21 (check them with a +90° Z-rotation test and a parent-child test).
5. glTF and JSON #16–18 and #22–23.
6. Tier 2, then Tier 3 as time allows.

## Verification
- `meson compile -C builddir` after each group. Run `darray_test` (`src/binaries/testing/darray_test.c`) and add cases for insert at the front, insert at `length`, and `get_pointer` at index 2 or higher.
- Run the game with Vulkan validation layers on: register two or more meshes and objects, resize, minimize and restore, then close the window and confirm it exits cleanly.
- Load a glTF with interleaved attributes and more than 512 objects, and a JSON file with `null` and integer arrays.
- Run under ASan (`-Db_sanitize=address`) to catch the remaining overflows and leaks.
