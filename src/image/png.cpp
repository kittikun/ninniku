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
#include "ninniku/image/png.h"

#include "../utils/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ninniku {
    pngImage::~pngImage()
    {
        if (_data != nullptr)
            stbi_image_free(_data);
    }

    TextureParam pngImage::CreateTextureParam(uint8_t viewFlags) const
    {
        throw std::exception("not implemented");
    }

    bool pngImage::Load(const std::string& path)
    {
        auto fmt = boost::format("pngImage::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        _data = stbi_load(path.c_str(), (int*)&_width, (int*)&_height, (int*)&_bpp, 0);

        return true;
    }

    std::tuple<uint8_t*, uint32_t> pngImage::GetData() const
    {
        uint32_t size = _width * _height * _bpp;

        return std::make_tuple(_data, size);
    }

    std::vector<SubresourceParam> pngImage::GetInitializationData() const
    {
        throw std::exception("not implemented");
    }

    void pngImage::InitializeFromTextureObject(std::unique_ptr<DX11>& dx, const std::unique_ptr<TextureObject>& srcTex)
    {
        throw std::exception("not implemented");
    }

    void pngImage::UpdateSubImage(uint32_t dstFace, uint32_t dstMip, uint8_t* newData, uint32_t newRowPitch)
    {
        throw std::exception("not implemented");
    }
} // namespace ninniku