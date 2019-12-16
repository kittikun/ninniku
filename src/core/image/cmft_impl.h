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

#include "ninniku/core/image/cmft.h"

#include <cmft/image.h>

namespace ninniku
{
    class cmftImageImpl final : public ImageImpl
    {
        // no copy of any kind allowed
        cmftImageImpl(const cmftImageImpl&) = delete;
        cmftImageImpl& operator=(cmftImageImpl&) = delete;
        cmftImageImpl(cmftImageImpl&&) = delete;
        cmftImageImpl& operator=(cmftImageImpl&&) = delete;

    public:
        cmftImageImpl() = default;
        ~cmftImageImpl();

        const std::tuple<uint8_t*, uint32_t> GetData() const override;

        // Used when transferring data back from the GPU
        void InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex) override;

        bool LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format) override;

        bool SaveImage(const std::filesystem::path& path, cmftImage::SaveType type);

    protected:
        TextureParamHandle CreateTextureParamInternal(const EResourceViews viewFlags) const override;
        uint32_t GetHeight() const override { return _image.m_height; }
        const std::vector<SubresourceParam> GetInitializationData() const override;
        uint32_t GetWidth() const override { return _image.m_width; }
        bool LoadInternal(const std::string& path) override;
        void UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch) override;
        bool ValidateExtension(const std::string& ext) const override;

    private:
        void AllocateMemory();
        bool AssembleCubemap();
        bool LoadEXR(const std::filesystem::path& path);
        cmft::TextureFormat::Enum GetFormatFromNinnikuFormat(uint32_t format) const;
        cmft::ImageFileType::Enum GetFiletypeFromFilename(const std::filesystem::path& path);
        uint32_t GetBPPFromFormat(cmft::TextureFormat::Enum format) const;

    private:
        cmft::Image _image;
    };
} // namespace ninniku
