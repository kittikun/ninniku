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

#ifndef NO_PCH
#include "pch.h"
#endif

#include "generic_impl.h"

#include "ninniku/core/image/generic.h"

#include "../../utils/log.h"

#pragma warning(push)
#pragma warning(disable:6011 6308 6262 28182)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma warning(pop)

#include <boost/format.hpp>
#include <array>
#include <filesystem>
#include <DirectXPackedVector.h>

namespace ninniku
{
    genericImage::genericImage()
        : impl_{ new genericImageImpl() }
    {
    }

    genericImage::~genericImage() = default;

    genericImageImpl::~genericImageImpl()
    {
        Reset();
    }

    void genericImageImpl::ConvertToR11G11B10()
    {
        auto size = width_ * height_;

        convertedData_.resize(size);

        for (uint32_t i = 0; i < size; ++i) {
            float r, g, b;

            if (data16_ != nullptr) {
                r = static_cast<float>(data16_[i * 3 + 0]);
                g = static_cast<float>(data16_[i * 3 + 1]);
                b = static_cast<float>(data16_[i * 3 + 2]);
            } else {
                r = static_cast<float>(data8_[i * 3 + 0]) / 255.f;
                g = static_cast<float>(data8_[i * 3 + 1]) / 255.f;
                b = static_cast<float>(data8_[i * 3 + 2]) / 255.f;
            }

            auto r11b11g10 = DirectX::PackedVector::XMFLOAT3PK(r, g, b);

            convertedData_[i] = r11b11g10;
        }
    }

    TextureParamHandle genericImageImpl::CreateTextureParamInternal(const EResourceViews viewFlags) const
    {
        auto res = TextureParam::Create();

        auto fmt = GetFormat();

        if (fmt == TF_UNKNOWN)
            return std::move(res);

        res->format = fmt;
        res->arraySize = 1;
        res->depth = 1;
        res->numMips = 1;
        res->width = width_;
        res->height = height_;
        res->imageDatas = GetInitializationData();
        res->viewflags = viewFlags;

        return std::move(res);
    }

    ETextureFormat genericImageImpl::GetFormat() const
    {
        auto res = TF_UNKNOWN;

        switch (bpp_) {
            case 4:
                if (data16_ != nullptr)
                    res = TF_R16G16B16A16_UNORM;
                else
                    res = TF_R8G8B8A8_UNORM;
                break;

            case 3:
                if (data16_ != nullptr)
                    res = TF_R16G16B16A16_UNORM;
                else
                    res = TF_R11G11B10_FLOAT;
                break;

            case 2:
                if (data16_ != nullptr)
                    res = TF_R16G16_UNORM;
                else
                    res = TF_R8G8_UNORM;
                break;

            case 1:
                if (data16_ != nullptr)
                    res = TF_R16_UNORM;
                else
                    res = TF_R8_UNORM;
                break;

            default:
                LOGE << "Unsupported format";
        }

        return res;
    }

    bool genericImageImpl::LoadInternal(const std::string_view& path)
    {
        auto fmt = boost::format("genericImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        Reset();

        if (stbi_is_16_bit(path.data()))
            data16_ = stbi_load_16(path.data(), (int*)&width_, (int*)&height_, (int*)&bpp_, 0);
        else
            data8_ = stbi_load(path.data(), (int*)&width_, (int*)&height_, (int*)&bpp_, 0);

        if (bpp_ == 3) {
            // we must convert from RGB to R11G11B10 since there is no R8G8B8 formats
            // a bit overkill but better than having an unused alpha
            ConvertToR11G11B10();
        }

        return true;
    }

    bool genericImageImpl::LoadRaw([[maybe_unused]] const void* pData, [[maybe_unused]] const size_t size, [[maybe_unused]] const uint32_t width, [[maybe_unused]] const uint32_t height, [[maybe_unused]] const int32_t format)
    {
        throw std::exception("not implemented");
    }

    const std::tuple<uint8_t*, uint32_t> genericImageImpl::GetData() const
    {
        uint32_t size = width_ * height_ * bpp_;

        if (data16_ != nullptr)
            return { reinterpret_cast<uint8_t*>(data16_), size * 2 };

        return { data8_, size };
    }

    const std::vector<SubresourceParam> genericImageImpl::GetInitializationData() const
    {
        std::vector<SubresourceParam> res(1);

        if (bpp_ == 3) {
            res[0].data = const_cast<uint32_t*>(convertedData_.data());
            res[0].rowPitch = width_ * sizeof(uint32_t);
        } else {
            if (data16_ != nullptr) {
                res[0].data = data16_;
                res[0].rowPitch = width_ * bpp_ * 2;
            } else {
                res[0].data = data8_;
                res[0].rowPitch = width_ * bpp_;
            }
        }

        res[0].depthPitch = 0;

        return res;
    }

    bool genericImageImpl::InitializeFromTextureObject([[maybe_unused]] RenderDeviceHandle& dx, [[maybe_unused]] const TextureObject* srcTex)
    {
        throw std::exception("not implemented");
    }

    void genericImageImpl::Reset()
    {
        width_ = height_ = bpp_ = 0;

        if (data8_ != nullptr) {
            stbi_image_free(data8_);
            data8_ = nullptr;
        }

        if (data16_ != nullptr) {
            stbi_image_free(data16_);
            data16_ = nullptr;
        }
    }

    void genericImageImpl::UpdateSubImage([[maybe_unused]] const uint32_t dstFace, [[maybe_unused]] const uint32_t dstMip, [[maybe_unused]] const uint8_t* newData, [[maybe_unused]] const uint32_t newRowPitch)
    {
        throw std::exception("not implemented");
    }

    bool genericImageImpl::ValidateExtension(const std::string_view& ext) const
    {
        const std::array<std::string_view, 9> valid = { ".jpg", ".png", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic", ".pnm" };

        for (auto& validExt : valid)
            if (ext == validExt)
                return true;

        auto fmt = boost::format("genericImage does not support extension: \"%1%\"") % ext;
        LOGE << boost::str(fmt);

        return false;
    }
} // namespace ninniku