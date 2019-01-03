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
#include "png_impl.h"

#include "../utils/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ninniku {
    pngImageImpl::~pngImageImpl()
    {
        if (_data != nullptr)
            stbi_image_free(_data);
    }

    TextureParam pngImageImpl::CreateTextureParam(const ETextureViews viewFlags) const
    {
        TextureParam res = {};

        switch (_bpp) {
            case 4:
                res.format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            case 3:
                res.format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            default:
                LOGE << "Unsupported PNG format";
                return res;
        }

        res.arraySize = 1;
        res.depth = 1;
        res.numMips = 1;
        res.width = _width;
        res.height = _height;
        res.imageDatas = GetInitializationData();
        res.viewflags = viewFlags;

        return res;
    }

    bool pngImageImpl::Load(const std::string& path)
    {
        auto fmt = boost::format("pngImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        _data = stbi_load(path.c_str(), (int*)&_width, (int*)&_height, (int*)&_bpp, 0);

        return true;
    }

    std::tuple<uint8_t*, uint32_t> pngImageImpl::GetData() const
    {
        uint32_t size = _width * _height * _bpp;

        return std::make_tuple(_data, size);
    }

    std::vector<SubresourceParam> pngImageImpl::GetInitializationData() const
    {
        std::vector<SubresourceParam> res(1);

        res[0].data = _data;
        res[0].rowPitch = _width * _bpp;
        res[0].depthPitch = 0;

        return res;
    }

    void pngImageImpl::InitializeFromTextureObject(std::unique_ptr<DX11>& dx, const std::unique_ptr<TextureObject>& srcTex)
    {
        throw std::exception("not implemented");
    }

    void pngImageImpl::UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch)
    {
        throw std::exception("not implemented");
    }
} // namespace ninniku