// Copyright(c) 2018-2020 Kitti Vongsay
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

#include "../../types.h"
#include "../renderer/renderdevice.h"
#include "../renderer/types.h"

namespace ninniku
{
    using SizeFixResult = std::tuple<bool, uint32_t, uint32_t>;

    class Image
    {
        // no copy of any kind allowed
        Image(const Image&) = delete;
        Image& operator=(Image&) = delete;
        Image(Image&&) = delete;
        Image& operator=(Image&&) = delete;

    public:
        Image() = default;
        virtual ~Image() = default;

        virtual bool Load(const std::string&) = 0;
        virtual bool LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format) = 0;
        virtual TextureParamHandle CreateTextureParam(const EResourceViews viewFlags) const = 0;

        virtual const std::tuple<uint8_t*, uint32_t> GetData() const { return std::tuple<uint8_t*, uint32_t>(); }

        // Used when transferring data back from the GPU
        virtual void InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex) = 0;

        /// <summary>
        /// Check if a image is a power of 2 and return a 2 item tuple
        /// 0 bool: need fix
        /// 1 uint32: new width
        /// 2 uint32: new height
        /// </summary>
        virtual const SizeFixResult IsRequiringFix() const = 0;
    };
} // namespace ninniku
