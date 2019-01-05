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

namespace ninniku {
    class pngImageImpl final : public ImageImpl
    {
        // no copy of any kind allowed
        pngImageImpl(const pngImageImpl&) = delete;
        pngImageImpl& operator=(pngImageImpl&) = delete;
        pngImageImpl(pngImageImpl&&) = delete;
        pngImageImpl& operator=(pngImageImpl&&) = delete;

    public:
        pngImageImpl() = default;
        ~pngImageImpl();

        TextureParamHandle CreateTextureParam(const ETextureViews viewFlags) const override;
        const bool Load(const std::string&) override;
        const std::tuple<uint8_t*, uint32_t> GetData() const override;

        // Used when transfering data back from the GPU
        void InitializeFromTextureObject(DX11Handle& dx, const TextureHandle& srcTex) override;

        // Save Image as DDS R32G32B32A32_FLOAT
        bool SaveImage(const std::string&);

    protected:
        const uint32_t GetHeight() const override { return _height; }
        const std::vector<SubresourceParam> GetInitializationData() const override;
        const uint32_t GetWidth() const override { return _width; }
        void UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch) override;

    private:
        void ConvertToR11G11B10();
    private:
        uint32_t _width;
        uint32_t _height;
        uint32_t _bpp;
        uint8_t* _data;
        std::vector<uint32_t> _convertedData;
    };
} // namespace ninniku
