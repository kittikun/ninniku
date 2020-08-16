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

#include "image_impl.h"

#include <DirectXTex.h>

namespace ninniku
{
    class ddsImageImpl final : public ImageImpl
    {
        // no copy of any kind allowed
        ddsImageImpl(const ddsImageImpl&) = delete;
        ddsImageImpl& operator=(ddsImageImpl&) = delete;
        ddsImageImpl(ddsImageImpl&&) = delete;
        ddsImageImpl& operator=(ddsImageImpl&&) = delete;

    public:
        ddsImageImpl() = default;

        const std::tuple<uint8_t*, uint32_t> GetData() const override;

        bool InitializeFromSwapChain(RenderDeviceHandle& dx, const SwapChainHandle& src);

        // Used when transferring data back from the GPU
        bool InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureObject* srcTex) override;

        bool LoadRaw(const void* pData, const size_t size);
        bool LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format) override;

        bool SaveImage(const std::string_view&);
        bool SaveCompressedImage(const std::string_view&, RenderDeviceHandle& dx, DXGI_FORMAT format);

    protected:
        TextureParamHandle CreateTextureParamInternal(const EResourceViews viewFlags) const override;
        uint32_t GetHeight() const override { return static_cast<uint32_t>(meta_.height); }
        const std::vector<SubresourceParam> GetInitializationData() const override;
        uint32_t GetWidth() const override { return static_cast<uint32_t>(meta_.width); }
        bool LoadInternal(const std::string_view& path) override;
        void UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch) override;
        bool ValidateExtension(const std::string_view& ext) const override;

    private:
        DirectX::TexMetadata meta_;
        DirectX::ScratchImage scratch_;
    };
} // namespace ninniku