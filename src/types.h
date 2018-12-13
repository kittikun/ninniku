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