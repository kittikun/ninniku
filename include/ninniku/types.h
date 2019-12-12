// Copyright(c) 2018-2019 Kitti Vongsay
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "export.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ninniku
{
    static constexpr uint32_t CUBEMAP_NUM_FACES = 6;

    class BufferObject;
    class TextureObject;

    //////////////////////////////////////////////////////////////////////////
    // Buffers Params
    //////////////////////////////////////////////////////////////////////////
    struct BufferParam
    {
        // no copy of any kind allowed, use Duplicate for that
        BufferParam(const BufferParam&) = delete;
        BufferParam& operator=(BufferParam&) = delete;
        BufferParam(BufferParam&&) = delete;
        BufferParam& operator=(BufferParam&&) = delete;
    public:
        BufferParam() = default;

        uint32_t numElements;
        uint32_t elementSize;        // If != 0, this will create a StructuredBuffer, otherwise a ByteAddressBuffer will be created
        uint8_t viewflags;

        static NINNIKU_API std::shared_ptr<BufferParam> Create();
        NINNIKU_API std::shared_ptr<BufferParam> Duplicate() const;
    };

    using BufferParamHandle = std::shared_ptr<const BufferParam>;

    //////////////////////////////////////////////////////////////////////////
    // Resources
    //////////////////////////////////////////////////////////////////////////
    enum EResourceViews : uint8_t
    {
        RV_None = 0,            // Should not be used
        RV_SRV = 1 << 0,
        RV_UAV = 1 << 1,
        RV_CPU_READ = 1 << 2
    };

    struct CopyBufferSubresourceParam
    {
        // no copy of any kind allowed
        CopyBufferSubresourceParam(const CopyBufferSubresourceParam&) = delete;
        CopyBufferSubresourceParam& operator=(CopyBufferSubresourceParam&) = delete;
        CopyBufferSubresourceParam(CopyBufferSubresourceParam&&) = delete;
        CopyBufferSubresourceParam& operator=(CopyBufferSubresourceParam&&) = delete;

        const BufferObject* src;
        const BufferObject* dst;
    };

    struct CopyTextureSubresourceParam
    {
        // no copy of any kind allowed
        CopyTextureSubresourceParam(const CopyTextureSubresourceParam&) = delete;
        CopyTextureSubresourceParam& operator=(CopyTextureSubresourceParam&) = delete;
        CopyTextureSubresourceParam(CopyTextureSubresourceParam&&) = delete;
        CopyTextureSubresourceParam& operator=(CopyTextureSubresourceParam&&) = delete;

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
    enum class ESamplerState : uint8_t
    {
        SS_Point,
        SS_Linear,
        SS_Count
    };

    //////////////////////////////////////////////////////////////////////////
    // Textures
    //////////////////////////////////////////////////////////////////////////

    enum ETextureFormat : uint8_t
    {
        TF_UNKNOWN,
        TF_R8_UNORM,
        TF_R8G8_UNORM,
        TF_R8G8B8A8_UNORM,
        TF_R11G11B10_FLOAT,
        TF_R16_UNORM,
        TF_R16G16_UNORM,
        TF_R16G16B16A16_UNORM,
        TF_R32G32B32A32_FLOAT
    };

    class TextureParam
    {
        // no copy of any kind allowed, use Duplicate for that
        TextureParam(const TextureParam&) = delete;
        TextureParam& operator=(TextureParam&) = delete;
        TextureParam(TextureParam&&) = delete;
        TextureParam& operator=(TextureParam&&) = delete;
    public:
        TextureParam() = default;

        static NINNIKU_API std::shared_ptr<TextureParam> Create();
        NINNIKU_API std::shared_ptr<TextureParam> Duplicate() const;

    public:
        uint32_t numMips;
        uint32_t arraySize;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t viewflags;
        uint32_t format;

        // one per face/mip/array etc..
        std::vector<SubresourceParam> imageDatas;
    };

    using TextureParamHandle = std::shared_ptr<const TextureParam>;
} // namespace ninniku