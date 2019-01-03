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

#include "../export.h"
#include "../types.h"

namespace ninniku {
    class DX11;
    struct TextureObject;

    class Image
    {
        // no copy of any kind allowed
        Image(const Image&) = delete;
        Image& operator=(Image&) = delete;
        Image(Image&&) = delete;
        Image& operator=(Image&&) = delete;

    public:
        Image() = default;
        virtual ~Image() = default;

        virtual bool Load(const std::string&) = 0;
        virtual TextureParam CreateTextureParam(const ETextureViews viewFlags) const = 0;

        virtual std::tuple<uint8_t*, uint32_t> GetData() const { return std::tuple<uint8_t*, uint32_t>(); }

        // Used when transfering data back from the GPU
        virtual void InitializeFromTextureObject(std::unique_ptr<DX11>& dx, const std::unique_ptr<TextureObject>& srcTex) = 0;
    };
} // namespace ninniku
