#pragma once

#include <bgfx/bgfx.h>

#define BGFX_CAPS_NATIVE_PASS UINT64_C(0x0000000800000000)
#define BGFX_CAPS_RAY_QUERY UINT64_C(0x0000001000000000)

namespace bgfx
{
    constexpr uint8_t kNativePassMaxTextures = 12;
    constexpr uint16_t kNativePassMaxPayload = 256;

    struct NativeTextureBinding
    {
        TextureHandle handle;
        Access::Enum access;
    };

    struct NativePassContext
    {
        RendererType::Enum rendererType;
        void *device;
        void *commandList;
        void *completionFence;
        uint64_t completionValue;
        uint32_t frame;
        void *textures[kNativePassMaxTextures];
        uint8_t textureCount;
    };

    using NativePassCallback = void (*)(const NativePassContext &context, const void *payload) noexcept;

    // API-thread only; payload bytes are copied into this frame. See docs/native-passes.md for resource lifetime and state rules.
    bool submitNative(ViewId view, NativePassCallback callback, const void *payload, uint16_t payloadSize, const NativeTextureBinding *textures = nullptr, uint8_t textureCount = 0);
}
