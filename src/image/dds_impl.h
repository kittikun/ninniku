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

#include "image_Impl.h"

#include <DirectXTex.h>

namespace ninniku {
    class ddsImageImpl final : public ImageImpl
    {
        // no copy of any kind allowed
        ddsImageImpl(const ddsImageImpl&) = delete;
        ddsImageImpl& operator=(ddsImageImpl&) = delete;
        ddsImageImpl(ddsImageImpl&&) = delete;
        ddsImageImpl& operator=(ddsImageImpl&&) = delete;

    public:
        ddsImageImpl() = default;

        TextureParamHandle CreateTextureParam(const ETextureViews viewFlags) const override;
        const bool Load(const std::string&) override;
        const std::tuple<uint8_t*, uint32_t> GetData() const override;

        // Used when transfering data back from the GPU
        void InitializeFromTextureObject(DX11Handle& dx, const TextureHandle& srcTex) override;

        bool SaveImage(const std::string&, DX11Handle& dx, DXGI_FORMAT format);

    protected:
        const uint32_t GetHeight() const override { return static_cast<uint32_t>(_meta.height); }
        const std::vector<SubresourceParam> GetInitializationData() const override;
        const uint32_t GetWidth() const override { return static_cast<uint32_t>(_meta.width); }
        void UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch) override;

    private:
        DirectX::TexMetadata _meta;
        DirectX::ScratchImage _scratch;
    };
} // namespace ninniku