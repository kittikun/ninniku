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

#include <stdint.h>
#include <string>

namespace ninniku
{
    class DX11;

    enum ELogLevel : uint8_t
    {
        LL_NONE,
        LL_WARN_ERROR,
        LL_NORMAL,
        LL_FULL
    };

    enum ERenderer : uint8_t
    {
        RENDERER_DX11,
        RENDERER_WARP
    };

    /// <summary>
    /// Initialize niniku framework
    /// shaderPath must point to compiled .cso folder
    /// </summary>
    bool Initialize(uint8_t renderer, const std::string& shaderPath, uint8_t logLevel = LL_WARN_ERROR);

    std::unique_ptr<DX11>& GetRenderer();
} // namespace ninniku
