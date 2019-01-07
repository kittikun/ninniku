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

#include "ninniku/image/image.h"

namespace ninniku {
    class ImageImpl : public Image
    {
        // no copy of any kind allowed
        ImageImpl(const ImageImpl&) = delete;
        ImageImpl& operator=(ImageImpl&) = delete;
        ImageImpl(ImageImpl&&) = delete;
        ImageImpl& operator=(ImageImpl&&) = delete;

    public:
        ImageImpl() = default;
        virtual ~ImageImpl() = default;

        virtual const bool Load(const std::string&) = 0;
        virtual TextureParamHandle CreateTextureParam(const ETextureViews viewFlags) const = 0;

        virtual const std::tuple<uint8_t*, uint32_t> GetData() const { return std::tuple<uint8_t*, uint32_t>(); }

        virtual void InitializeFromTextureObject(DX11Handle& dx, const TextureHandle& srcTex) = 0;

        const SizeFixResult IsRequiringFix() const override;

    protected:
        virtual const std::vector<SubresourceParam> GetInitializationData() const = 0;
        virtual void UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch) = 0;
        virtual const uint32_t GetWidth() const = 0;
        virtual const uint32_t GetHeight() const = 0;
    };
} // namespace ninniku
