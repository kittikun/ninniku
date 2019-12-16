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

#include "pch.h"
#include "generic_impl.h"

#include "ninniku/core/image/generic.h"

#include "../../utils/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <array>
#include <filesystem>
#include <DirectXPackedVector.h>

namespace ninniku
{
    genericImage::genericImage()
        : _impl{ new genericImageImpl() }
    {
    }

    genericImage::~genericImage() = default;

    genericImageImpl::~genericImageImpl()
    {
        Reset();
    }

    void genericImageImpl::ConvertToR11G11B10()
    {
        auto size = _width * _height;

        _convertedData.resize(size);

        for (uint32_t i = 0; i < size; ++i) {
            float r, g, b;

            if (_data16 != nullptr) {
                r = static_cast<float>(_data16[i * 3 + 0]);
                g = static_cast<float>(_data16[i * 3 + 1]);
                b = static_cast<float>(_data16[i * 3 + 2]);
            } else {
                r = static_cast<float>(_data8[i * 3 + 0]) / 255.f;
                g = static_cast<float>(_data8[i * 3 + 1]) / 255.f;
                b = static_cast<float>(_data8[i * 3 + 2]) / 255.f;
            }

            auto r11b11g10 = DirectX::PackedVector::XMFLOAT3PK(r, g, b);

            _convertedData[i] = r11b11g10;
        }
    }

    TextureParamHandle genericImageImpl::CreateTextureParamInternal(const EResourceViews viewFlags) const
    {
        auto res = std::make_shared<TextureParam>();

        auto fmt = GetFormat();

        if (fmt == TF_UNKNOWN)
            return std::move(res);

        res->format = fmt;
        res->arraySize = 1;
        res->depth = 1;
        res->numMips = 1;
        res->width = _width;
        res->height = _height;
        res->imageDatas = GetInitializationData();
        res->viewflags = viewFlags;

        return std::move(res);
    }

    ETextureFormat genericImageImpl::GetFormat() const
    {
        auto res = TF_UNKNOWN;

        switch (_bpp) {
            case 4:
                if (_data16 != nullptr)
                    res = TF_R16G16B16A16_UNORM;
                else
                    res = TF_R8G8B8A8_UNORM;
                break;

            case 3:
                if (_data16 != nullptr)
                    res = TF_R16G16B16A16_UNORM;
                else
                    res = TF_R11G11B10_FLOAT;
                break;

            case 2:
                if (_data16 != nullptr)
                    res = TF_R16G16_UNORM;
                else
                    res = TF_R8G8_UNORM;
                break;

            case 1:
                if (_data16 != nullptr)
                    res = TF_R16_UNORM;
                else
                    res = TF_R8_UNORM;
                break;

            default:
                LOGE << "Unsupported format";
        }

        return res;
    }

    bool genericImageImpl::LoadInternal(const std::string& path)
    {
        auto fmt = boost::format("genericImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        Reset();

        if (stbi_is_16_bit(path.c_str()))
            _data16 = stbi_load_16(path.c_str(), (int*)&_width, (int*)&_height, (int*)&_bpp, 0);
        else
            _data8 = stbi_load(path.c_str(), (int*)&_width, (int*)&_height, (int*)&_bpp, 0);

        if (_bpp == 3) {
            // we must convert from RGB to R11G11B10 since there is no R8G8B8 formats
            // a bit overkill but better than having an unused alpha
            ConvertToR11G11B10();
        }

        return true;
    }

    bool genericImageImpl::LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format)
    {
        throw std::exception("not implemented");
    }

    const std::tuple<uint8_t*, uint32_t> genericImageImpl::GetData() const
    {
        uint32_t size = _width * _height * _bpp;

        if (_data16 != nullptr)
            return std::make_tuple(reinterpret_cast<uint8_t*>(_data16), size * 2);

        return std::make_tuple(_data8, size);
    }

    const std::vector<SubresourceParam> genericImageImpl::GetInitializationData() const
    {
        std::vector<SubresourceParam> res(1);

        if (_bpp == 3) {
            res[0].data = const_cast<uint32_t*>(_convertedData.data());
            res[0].rowPitch = _width * sizeof(uint32_t);
        } else {
            if (_data16 != nullptr) {
                res[0].data = _data16;
                res[0].rowPitch = _width * _bpp * 2;
            } else {
                res[0].data = _data8;
                res[0].rowPitch = _width * _bpp;
            }
        }

        res[0].depthPitch = 0;

        return res;
    }

    void genericImageImpl::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex)
    {
        throw std::exception("not implemented");
    }

    void genericImageImpl::Reset()
    {
        _width = _height = _bpp = 0;

        if (_data8 != nullptr) {
            stbi_image_free(_data8);
            _data8 = nullptr;
        }

        if (_data16 != nullptr) {
            stbi_image_free(_data16);
            _data16 = nullptr;
        }
    }

    void genericImageImpl::UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch)
    {
        throw std::exception("not implemented");
    }

    bool genericImageImpl::ValidateExtension(const std::string& ext) const
    {
        const std::array<std::string, 9> valid = { ".jpg", ".png", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic", ".pnm" };

        for (auto& validExt : valid)
            if (ext == validExt)
                return true;

        auto fmt = boost::format("genericImage does not support extension: \"%1%\"") % ext;
        LOGE << boost::str(fmt);

        return false;
    }
} // namespace ninniku