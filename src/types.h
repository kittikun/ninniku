#pragma once

namespace ninniku {
    static constexpr uint32_t CUBEMAP_NUM_FACES = 6;

    struct TextureObject;

    //////////////////////////////////////////////////////////////////////////
    // Resources
    //////////////////////////////////////////////////////////////////////////
    struct CopySubresourceParam
    {
        const TextureObject* src;
        uint32_t srcFace;
        uint32_t srcMip;
        const TextureObject* dst;
        uint32_t dstFace;
        uint32_t dstMip;
    };

    struct SubresourceParam
    {
        void* data;

        // No need for 1D textures
        uint32_t rowPitch;

        // only for 3D textures
        uint32_t depthPitch;
    };

    //////////////////////////////////////////////////////////////////////////
    // Shader
    //////////////////////////////////////////////////////////////////////////
    enum ESamplerState : uint8_t
    {
        SS_Point,
        SS_Linear,
        SS_Count
    };

    //////////////////////////////////////////////////////////////////////////
    // Textures
    //////////////////////////////////////////////////////////////////////////
    enum ETextureViews : uint8_t
    {
        TV_None = 0,
        TV_SRV = 1 << 0,
        TV_UAV = 1 << 1,
        TV_CPU_READ = 1 << 2
    };

    struct TextureParam
    {
        uint32_t numMips;
        uint32_t arraySize;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint8_t viewflags;
        uint32_t format;

        // one per face/mip/array etc..
        std::vector<SubresourceParam> imageDatas;
    };
} // namespace ninniku