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

#include "ninniku/Image/generic.h"

#include "../utils/log.h"

#include <boost/filesystem.hpp>
#include <DirectXPackedVector.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ninniku
{
    genericImage::genericImage()
        : _impl{ new genericImageImpl() }
    {
    }

    genericImage::~genericImage() = default;

    genericImageImpl::~genericImageImpl()
    {
        if (_data != nullptr)
            stbi_image_free(_data);
    }

    void genericImageImpl::ConvertToR11G11B10()
    {
        auto size = _width * _height;

        _convertedData.resize(size);

        for (uint32_t i = 0; i < size; ++i) {
            float r = static_cast<float>(_data[i * 3 + 0]) / 255.f;
            float g = static_cast<float>(_data[i * 3 + 1]) / 255.f;
            float b = static_cast<float>(_data[i * 3 + 2]) / 255.f;
            auto r11b11g10 = DirectX::PackedVector::XMFLOAT3PK(r, g, b);

            _convertedData[i] = r11b11g10;
        }
    }

    TextureParamHandle genericImageImpl::CreateTextureParam(const uint8_t viewFlags) const
    {
        auto res = std::make_shared<TextureParam>();

        switch (_bpp) {
            case 4:
                res->format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            case 3:
                res->format = DXGI_FORMAT_R11G11B10_FLOAT;
                break;

            case 1:
                res->format = DXGI_FORMAT_R8_UNORM;
                break;

            default:
                LOGE << "Unsupported PNG format";
                return std::move(res);
        }

        res->arraySize = 1;
        res->depth = 1;
        res->numMips = 1;
        res->width = _width;
        res->height = _height;
        res->imageDatas = GetInitializationData();
        res->viewflags = viewFlags;

        return res;
    }

    bool genericImageImpl::LoadInternal(const std::string& path)
    {
        auto fmt = boost::format("genericImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        int forcedSize = 0;

        _data = stbi_load(path.c_str(), (int*)&_width, (int*)&_height, (int*)&_bpp, 0);

        if (_bpp == 3) {
            // we must convert from RGB to R11G11B10 since there is no R8G8B8 formats
            // a bit overkill but better than having an unused alpha
            ConvertToR11G11B10();
        }

        return true;
    }

    const std::tuple<uint8_t*, uint32_t> genericImageImpl::GetData() const
    {
        uint32_t size = _width * _height * _bpp;

        return std::make_tuple(_data, size);
    }

    const std::vector<SubresourceParam> genericImageImpl::GetInitializationData() const
    {
        std::vector<SubresourceParam> res(1);

        if (_bpp == 3) {
            res[0].data = const_cast<uint32_t*>(_convertedData.data());
            res[0].rowPitch = _width * sizeof(uint32_t);
        } else {
            res[0].data = _data;
            res[0].rowPitch = _width * _bpp;
        }

        res[0].depthPitch = 0;

        return res;
    }

    void genericImageImpl::InitializeFromTextureObject(DX11Handle& dx, const TextureHandle& srcTex)
    {
        throw std::exception("not implemented");
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