# Rebuilding the Swapchain Without `vkDeviceWaitIdle`

Goal: resize the window and rebuild the swapchain (and the offscreen render target) while frames are still in flight, instead of stalling the whole GPU with [`vkDeviceWaitIdle`][waitidle].

The foundation is already there. The global timeline semaphore (`globalTimelineSemaphore` / `nextSubmitValue` in `VulkanEngine`) gives a per-frame "GPU has finished everything up to value N" signal. Everything below is about deferring destruction against that value rather than waiting idle. See [Khronos: Vulkan Timeline Semaphores][timeline-blog] for the model and [`vkGetSemaphoreCounterValue`][getcounter] for the host-side query we'll poll.

Estimated effort: one sitting.

Note that both [vkguide's "Resizable window"][vkguide-resize] and the [official tutorial's swapchain recreation chapter][tutorial-recreate] take the easy way out and call `vkDeviceWaitIdle`. Those pages are still useful for the *shape* of the rebuild, just not for the synchronization.

---

## Current state (what's wrong today)

| Place | Problem |
|---|---|
| `VulkanSwapchainResources::Recreate()` | Calls `vkDeviceWaitIdle`, then `ReleaseSwapchainResources()`, then creates fresh. |
| `VulkanSwapchainResources::ReleaseSwapchainResources()` | Also calls `vkDeviceWaitIdle`. |
| `CreateSwapchain(...)` | Passes `oldSwapchain = VK_NULL_HANDLE`. |
| `VulkanEngine::ResizeOffscreenRenderTarget()` | Destroys the offscreen image immediately, then creates the new one. Only safe because of the wait idle. |
| `VulkanEngine::ResizeOffscreenRenderTarget()` | Calls `vkUpdateDescriptorSets` on the single storage-image descriptor set. **Invalid once wait idle is gone** (see "The hidden gotcha"). |
| `DescriptorAllocator.cpp` | Pool has `maxSets = 1`, so no room to allocate a second set. |

Things that are already fine and don't need touching:

- The graphics pipeline only reads the offscreen image *format*, not the extent. Viewport and scissor are dynamic.
- The blit already scales from the offscreen extent to the swapchain extent, so a one-frame size mismatch renders a scaled frame instead of breaking.
- Per-frame `imageAcquiredSemaphore` and the timeline wait at the top of the frame are correct.

---

## Step 1: Timeline-keyed retirement queue

This is the only genuinely new mechanism. Everything after this step is "push the old thing into the queue instead of destroying it."

Add a queue where each entry carries a timeline value:

```cpp
struct RetiredEntry
{
    uint64_t safeAfterTimelineValue;
    std::function<void()> destroy;
};

class RetirementQueue
{
public:
    void Retire(uint64_t safeAfterValue, std::function<void()>&& fn);

    // Destroys every entry whose value <= completedValue.
    void Flush(uint64_t completedValue);

    // Destroys everything regardless of value. Used at shutdown after the final wait idle.
    void FlushAll();

private:
    std::deque<RetiredEntry> entries;
};
```

Wire it up:

- Own one `RetirementQueue` in `VulkanEngine`.
- At the top of `RenderAndPresentToSwapchain`, right after the existing `vkWaitSemaphores`, call [`vkGetSemaphoreCounterValue`][getcounter]`(device, globalTimelineSemaphore, &completed)` and `retirementQueue.Flush(completed)`.
- At shutdown, keep the existing `WaitForDeviceIdle()` in `SDL_AppQuit` and then `FlushAll()` before the main deletion queue runs.

Note: `FunctionalQueue` asserts if destroyed non-empty. Either give `RetirementQueue` the same behavior or make sure `FlushAll()` is always called on teardown.

This is the same idea as vkguide's [`DeletionQueue`][vkguide-deletionqueue], just with a "not before timeline value N" gate on each entry.

---

## Step 2: Per-frame-in-flight descriptor sets

Do this **before** removing the wait idle so it can be verified in isolation.

### The hidden gotcha

`ResizeOffscreenRenderTarget` updates the one shared descriptor set in place. With three frames in flight and no wait idle, that set is still referenced by up to two *pending* command buffers. The spec forbids this outright ([VUID-vkUpdateDescriptorSets-None-03047][updatedesc]):

> The `dstSet` member of each element of `pDescriptorWrites` or `pDescriptorCopies` for bindings which were created without the `VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT` or `VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT` bits set **must** not be used by any command that was recorded to a command buffer which is in the pending state.

Validation layers will flag it the moment the wait idle goes away. (Those two flags are from [`VkDescriptorBindingFlagBits`][bindingflags] / descriptor indexing. They're the "right" long-term answer for bindless-style setups but overkill here.)

### Recommendation: one set per frame in flight

Give each `PerFrameInFlightStructure` its own descriptor set, alongside its command buffer and acquire semaphore. At the top of each frame, after the timeline wait has proven that frame's previous command buffer is done, write the current offscreen image into *that frame's* set, then bind it.

Why this over "the image owns its set":

- **You'll need it anyway.** Anything that changes per frame (camera uniforms, scene data, per-frame buffers) needs a descriptor set that isn't being read by a pending command buffer. That is exactly the per-frame slot. vkguide builds this in [chapter 4, "Descriptor Abstractions"][vkguide-desc], as a growable per-frame allocator that is reset each frame. Setting up the per-frame slot now means that chapter drops straight in later.
- **It naturally handles the overlap.** Frame N's set points at the new image. Frames N-1 and N-2 still point at the old image, which is still alive because it's sitting in the retirement queue. Nothing has to track "which set points at which image."
- **The write is free.** One `vkUpdateDescriptorSets` with one storage-image write per frame is nothing. Don't bother trying to write only on change; that needs per-frame dirty tracking for no gain.

The trade-off, for honesty: long-lived resources (textures, material params) usually live in persistent sets that are written once, and the offscreen image is closer to that category than to per-frame data. So "image owns its set" is also a perfectly valid design and is a few lines less code. It just doesn't build anything you'll reuse.

### Concrete changes

- Add `VkDescriptorSet descriptorSet` to `PerFrameInFlightStructure`.
- `DescriptorAllocator`: bump `maxSets` and `descriptorCount` to `framesInFlight`. Allocate one set per frame in `CreateFrameInFlightStructures` (or in `init()` right after, since the pool lives in `descriptorInfo`).
- Freeing individual sets isn't needed. Destroying the pool frees them all. If you ever want per-set freeing, that's [`VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`][poolflags]; the per-frame reset pattern uses [`vkResetDescriptorPool`][resetpool] instead.
- In `RenderAndPresentToSwapchain`, after the timeline wait and after any `ResizeOffscreenRenderTarget`, call `UpdateOffscreenRenderImageDescriptorSet(device, offscreenRenderImageInfo, frameStruct.descriptorSet)`.
- `RenderToOffscreenImage` takes the frame's set and binds it via [`vkCmdBindDescriptorSets`][binddesc] instead of `descriptorInfo.descriptorSet`.
- Remove the standalone `descriptorSet` from `DescriptorInformation` (or leave it unused for now).

Run the app after this step with wait idle still in place. Nothing should look different.

Further reading on why per-frame sets are the normal pattern: [Vulkan Samples: Descriptor management][samples-descmgmt] compares the strategies and their costs.

---

## Step 3: Retire the offscreen image instead of destroying it

In `ResizeOffscreenRenderTarget`:

```cpp
// before
DestroyOffscreenRenderImageInfo(device, offscreenRenderImageInfo, vmaAllocator);
offscreenRenderImageInfo = createOffscreenRenderImageInfo(...);

// after
OffscreenRenderImageInfo old = offscreenRenderImageInfo;
offscreenRenderImageInfo = createOffscreenRenderImageInfo(...);
retirementQueue.Retire(nextSubmitValue - 1, [=, this] {
    DestroyOffscreenRenderImageInfo(gpuContext.device, old, vmaAllocator);
});
```

`nextSubmitValue - 1` is the value of the last submit that could have used the old image. Once the timeline reaches it, the old image is provably unused.

This is the "two render targets, one at the larger resolution" overlap. The old one stays alive for up to frames-in-flight frames and then goes away on its own.

Note `OffscreenRenderImageInfo` is a plain value struct, so copying it into the lambda is fine. Be careful not to accidentally destroy through the *new* struct.

---

## Step 4: Swapchain rebuild with `oldSwapchain` and retirement

### `CreateSwapchain`

- Add a `VkSwapchainKHR oldSwapchain` parameter and pass it through to [`VkSwapchainCreateInfoKHR::oldSwapchain`][swapchaincreate].
- Do **not** destroy the old swapchain inside this function.

What the spec says `oldSwapchain` buys you (from the page above):

> Providing a valid `oldSwapchain` may aid in the resource reuse, and also allows the application to still present any images that are already acquired from it.
>
> Upon calling `vkCreateSwapchainKHR` with an `oldSwapchain` that is not `VK_NULL_HANDLE`, `oldSwapchain` is retired, even if creation of the new swapchain fails.
>
> Multiple retired swapchains can be associated with the same `VkSurfaceKHR` through multiple uses of `oldSwapchain` that outnumber calls to `vkDestroySwapchainKHR`.

That last line is what makes a fast drag-resize safe: several retired swapchains can pile up in the retirement queue before the first one drains.

### `VulkanSwapchainResources`

- Delete both `vkDeviceWaitIdle` calls (`Recreate` and `ReleaseSwapchainResources`).
- `Recreate` becomes:
  1. Grab the old swapchain handle (or `VK_NULL_HANDLE` if none).
  2. Move the current `deletionQueue`, `swapchainImageViews`, and `swapchainImageReadyToPresentSemaphores` into a "retired bundle."
  3. Create the new swapchain, passing the old handle.
  4. Hand the retired bundle to the engine's `RetirementQueue`.

Since the class already has a `FunctionalQueue` holding the destroy lambdas, the simplest version is to `std::move` the whole queue into the retirement entry and call `Flush()` on it there. Watch out: those lambdas capture `this` and read `swapchainInfo` / the vectors by reference, so they'll see the *new* state by the time they run. Either capture the handles by value at registration time, or move the vectors into the retired bundle and have the lambdas close over the bundle.

`VulkanSwapchainResources` doesn't currently know about the engine's timeline. Options:
- Give `Recreate` an out-param / return value of "stuff to retire" and let `VulkanEngine` push it into the queue with the right value.
- Or pass a `std::function<void(std::function<void()>&&)>` retire callback into the swapchain resources at construction.

The first is simpler and keeps the swapchain class ignorant of the timeline.

### Which timeline value to retire at

| Resource | Retire at | Why |
|---|---|---|
| Offscreen image | `lastSubmit` | Timeline proves the command buffer that used it is done. |
| Swapchain image views | `lastSubmit` | Same. |
| Old `VkSwapchainKHR` handle | `lastSubmit + framesInFlight` | [`vkDestroySwapchainKHR`][destroyswapchain] requires "all uses of presentable images acquired from swapchain must have completed execution" (VUID 01282). Presents from the old swapchain may still be pending, and the timeline doesn't cover present. |
| Present semaphores (`imageReadyToPresent`) | `lastSubmit + framesInFlight` | [`vkQueuePresentKHR`][present] gives no host-visible signal for when it has consumed its wait semaphore. Padding by frames-in-flight is the standard workaround; see [Vulkan Guide: Swapchain Semaphore Reuse][semreuse]. |

Where `lastSubmit = nextSubmitValue - 1` at the moment of rebuild and `framesInFlight = 3`.

The proper fix for the present-semaphore problem is the present fence in [`VK_KHR_swapchain_maintenance1`][maint1] via [`VkSwapchainPresentFenceInfoKHR`][presentfence]. It's worth knowing about, but the extension isn't universally available and the frames-in-flight padding is what most engines ship with. Not needed for this pass.

---

## Step 5: Verify

- Run with validation layers on.
- Resize the window continuously for a while, including drag-resizing slowly and snapping between sizes.
- Minimize and restore (zero-extent path).
- Watch for `VUID-vkUpdateDescriptorSets-None-03047` (descriptor set in use), any "object still in use" errors on destroy, and semaphore reuse errors on acquire.

Step 4 is where anything wrong will surface. If validation is clean after a minute of abusive resizing, it's done.

---

## Edge cases to keep in mind

- **Acquire succeeds but the frame bails out.** If [`vkAcquireNextImageKHR`][acquire] returns an image and the frame does *not* submit a wait on `imageAcquiredSemaphore`, that semaphore stays pending-signal and the next acquire on it is invalid. The code doesn't do this today. Don't introduce it while refactoring. The [Vulkan Guide page on semaphore reuse][semreuse] covers this exact trap.
- **Zero-size extent.** `CreateSwapchain` returns `nullopt` on a zero extent, and `ResizeOffscreenRenderTarget` early-returns on zero. With retirement, nothing new gets retired on this path, which is fine. Make sure `GetExtent()` doesn't hand back a stale swapchain extent once `swapchainInfo` has been reset.
- **Multiple resizes before the queue drains.** With a fast drag you may retire several offscreen images and swapchains before the first one's value is reached. That's fine memory-wise for a few frames. The spec explicitly allows multiple retired swapchains on one surface (quoted above).
- **`SUBOPTIMAL` on acquire.** Currently sets `bNeedsRebuild` and still renders that frame. That behavior stays correct; the rebuild just happens on the next acquire without a stall.

---

## Order of work

1. `RetirementQueue` + flush at top of frame. (No behavior change yet.)
2. Per-frame descriptor sets, pool bumped, set written at top of frame. Run and confirm identical output.
3. `ResizeOffscreenRenderTarget` retires instead of destroys.
4. Swapchain: `oldSwapchain`, retirement of old handle/views/semaphores, delete both wait idle calls.
5. Resize-abuse test with validation layers.

---

## Sources

Spec pages (registry.khronos.org, always current):

- [`vkDeviceWaitIdle`][waitidle]
- [`vkGetSemaphoreCounterValue`][getcounter]
- [`vkUpdateDescriptorSets`][updatedesc] (VUID 03047 is the pending-command-buffer rule)
- [`VkDescriptorBindingFlagBits`][bindingflags] (update-after-bind, for later)
- [`VkDescriptorPoolCreateFlagBits`][poolflags]
- [`vkResetDescriptorPool`][resetpool]
- [`vkCmdBindDescriptorSets`][binddesc]
- [`VkSwapchainCreateInfoKHR`][swapchaincreate] (`oldSwapchain` semantics)
- [`vkDestroySwapchainKHR`][destroyswapchain] (VUID 01282)
- [`vkAcquireNextImageKHR`][acquire]
- [`vkQueuePresentKHR`][present]
- [`VK_KHR_swapchain_maintenance1`][maint1] and [`VkSwapchainPresentFenceInfoKHR`][presentfence]

Khronos guides and blogs:

- [Vulkan Timeline Semaphores (Khronos blog)][timeline-blog]
- [Vulkan Guide: Swapchain Semaphore Reuse][semreuse]
- [Vulkan-Docs wiki: Synchronization Examples][syncexamples] (see "Swapchain Image Acquire and Present")
- [Vulkan Samples: Descriptor management][samples-descmgmt]
- [Vulkan Tutorial: Swap chain recreation][tutorial-recreate] (uses wait idle; useful for the rebuild shape only)

vkguide (what this repo is loosely following):

- [Chapter 1: Mainloop code][vkguide-deletionqueue] (the `DeletionQueue` this retirement queue extends)
- [Chapter 3: Resizable window][vkguide-resize] (uses wait idle)
- [Chapter 4: Descriptor Abstractions][vkguide-desc] (per-frame growable descriptor allocator)

Other:

- [Yet another blog explaining Vulkan synchronization (Hans-Kristian Arntzen)][maister-sync]. Best single explanation of pipeline stages, access masks, and semaphores.

[waitidle]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkDeviceWaitIdle.html
[getcounter]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkGetSemaphoreCounterValue.html
[updatedesc]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkUpdateDescriptorSets.html
[bindingflags]: https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorBindingFlagBits.html
[poolflags]: https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorPoolCreateFlagBits.html
[resetpool]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkResetDescriptorPool.html
[binddesc]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdBindDescriptorSets.html
[swapchaincreate]: https://registry.khronos.org/vulkan/specs/latest/man/html/VkSwapchainCreateInfoKHR.html
[destroyswapchain]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkDestroySwapchainKHR.html
[acquire]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkAcquireNextImageKHR.html
[present]: https://registry.khronos.org/vulkan/specs/latest/man/html/vkQueuePresentKHR.html
[maint1]: https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_swapchain_maintenance1.html
[presentfence]: https://registry.khronos.org/vulkan/specs/latest/man/html/VkSwapchainPresentFenceInfoKHR.html
[timeline-blog]: https://www.khronos.org/blog/vulkan-timeline-semaphores
[semreuse]: https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
[syncexamples]: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
[samples-descmgmt]: https://docs.vulkan.org/samples/latest/samples/performance/descriptor_management/README.html
[tutorial-recreate]: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/04_Swap_chain_recreation.html
[vkguide-deletionqueue]: https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop_code/
[vkguide-resize]: https://vkguide.dev/docs/new_chapter_3/resizing_window/
[vkguide-desc]: https://vkguide.dev/docs/new_chapter_4/descriptor_abstractions/
[maister-sync]: https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
