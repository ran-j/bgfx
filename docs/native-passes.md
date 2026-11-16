# Native passes (PineBlack fork)

`<bgfx/native.h>` is an additive C++ extension. Existing bgfx entry points and the C API are unchanged. This extension currently targets static-library consumers and the D3D12 renderer; it is not a portable ray-tracing API.

`BGFX_CAPS_NATIVE_PASS` advertises ordered native callbacks. `BGFX_CAPS_RAY_QUERY` additionally requires DXR tier 1.1 and Shader Model 6.5 on the selected device. Other backends advertise neither capability and reject submissions. Neither flag enables an effect automatically.

## Submission

Call `submitNative(view, callback, payload, payloadSize, bindings, bindingCount)` on the API thread, after initialization and before `frame()`. It returns false for unsupported renderers, an occupied native slot, invalid view/callback/payload, invalid or duplicated texture handles, or write bindings without `BGFX_TEXTURE_COMPUTE_WRITE`. It schedules a touch automatically. Normal draw-call capacity limits and debug modes that disable rendering still apply.

One callback is supported per view per frame, with up to twelve texture bindings and 256 bytes of payload. The payload is copied into 16-byte-aligned frame storage without heap allocation; submit trivially copyable packets and decode them with `memcpy` into a local value. Embedded pointers are not retained or deep-copied: their owners must remain alive until callback execution. Never pass pointers to stack data or mutable simulation state. Callbacks are not retained for subsequent frames. Like `touch`, submission discards the current encoder bindings.

Execution follows `setViewOrder`, on the actual render thread, after earlier views and this view's blits, before this view's framebuffer clear and draw/compute items. Use a dedicated view for a native pass. Callbacks must be `noexcept`, must not reenter bgfx, and must not close/submit/reset the command list or modify the queue/fence. They record commands only. Errors must be communicated back to the owner explicitly.

## D3D12 context

- `device`: borrowed `ID3D12Device*`; query the required interface once during initialization.
- `commandList`: borrowed `ID3D12GraphicsCommandList*`, valid only during this callback. Query `ID3D12GraphicsCommandList4` for DXR commands.
- `textures`: borrowed `ID3D12Resource*` in binding order. Read bindings arrive in `GENERIC_READ`; write/read-write bindings arrive in `UNORDERED_ACCESS`. Do not transition these resources yourself. bgfx tracks their states and inserts a UAV barrier after the callback. The callback owns any descriptors it needs.
- `completionFence` and `completionValue`: borrowed `ID3D12Fence*` and the value that will be signalled after the command list is submitted. Do not signal this fence. Retain a COM reference if storing it beyond the callback. The value is a GPU completion token, not a frame counter.

Keep native resources, upload bytes and descriptor ranges alive and unchanged until the completion token is reached. The token covers this command list, not subsequent consumers: externally owned resources shared with later passes must remain alive through their last GPU use. Use a resource ring or deferred retirement; do not stall the CPU between render passes. `UINT64_MAX` returned by `GetCompletedValue()` means device removal, not successful completion. Resources created and used solely by native commands require their own D3D12 barriers. Retire native resources before renderer shutdown, or after shutdown has drained the queue while retaining their device references.

bgfx restores its framebuffer, viewport, scissor, pipeline, root signatures and descriptor heaps for following ordinary rendering. The extension uses the existing graphics queue; it does not add async compute or a separate submission thread. Draw and compute callbacks must restore any additional D3D12 state they introduce outside these bindings, such as predication.

PineBlack's `PineBlackNativePassTests` exercises a raster clear -> native DXR query -> raster draw -> readback sequence, reordered views, hit/miss distance, instance translation/masking, payload copying and one-frame callback lifetime in dedicated and main-thread modes. D3D11 and Vulkan cases check explicit rejection.
