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

#include "image.h"

namespace ninniku
{
    class pngImageImpl;

    class pngImage final : public Image
    {
        // no copy of any kind allowed
        pngImage(const pngImage&) = delete;
        pngImage& operator=(pngImage&) = delete;
        pngImage(pngImage&&) = delete;
        pngImage& operator=(pngImage&&) = delete;

    public:
        NINNIKU_API pngImage();
        NINNIKU_API ~pngImage();

        NINNIKU_API TextureParam CreateTextureParam(const ETextureViews viewFlags) const override;
        NINNIKU_API bool Load(const std::string&) override;
        NINNIKU_API std::tuple<uint8_t*, uint32_t> GetData() const override;

        // Used when transfering data back from the GPU
        NINNIKU_API void InitializeFromTextureObject(std::unique_ptr<DX11, DX11Deleter>& dx, const std::unique_ptr<TextureObject>& srcTex) override;

    private:
        std::unique_ptr<pngImageImpl> _impl;
    };
} // namespace ninniku
