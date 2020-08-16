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

#include "ninniku/core/renderer/types.h"

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////////////////////
    std::shared_ptr<BufferParam> BufferParam::Create()
    {
        return std::make_shared<BufferParam>();
    }

    std::shared_ptr<BufferParam> BufferParam::Duplicate() const
    {
        auto res = BufferParam::Create();

        res->elementSize = elementSize;
        res->numElements = numElements;
        res->viewflags = viewflags;

        return res;
    }

    //////////////////////////////////////////////////////////////////////////
    // Swap chain
    //////////////////////////////////////////////////////////////////////////
    std::shared_ptr<SwapchainParam> SwapchainParam::Create()
    {
        return std::make_shared<SwapchainParam>();
    }

    //////////////////////////////////////////////////////////////////////////
    // Textures
    //////////////////////////////////////////////////////////////////////////
    std::shared_ptr<TextureParam> TextureParam::Create()
    {
        return std::make_shared<TextureParam>();
    }

    std::shared_ptr<TextureParam> TextureParam::Duplicate() const
    {
        auto res = TextureParam::Create();

        res->arraySize = arraySize;
        res->depth = depth;
        res->format = format;
        res->height = height;
        res->numMips = numMips;
        res->viewflags = viewflags;
        res->width = width;
        res->imageDatas.reserve(imageDatas.size());
        res->imageDatas.assign(imageDatas.begin(), imageDatas.end());

        return res;
    }
} // namespace ninniku