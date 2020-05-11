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

#pragma once

#include "export.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace ninniku {
    enum  EInitializationFlags
    {
        IF_None = 0,
        IF_EnableCapture = 1 << 0,
        IF_DisableDX12DebugLayer = 1 << 1,
        IF_BC7_QUICK_MODE = 1 << 2
    };

    enum class ELogLevel : uint8_t
    {
        LL_NONE,
        LL_WARN_ERROR,
        LL_NORMAL,
        LL_FULL
    };

    enum ERenderer : uint8_t
    {
        RENDERER_WARP = 0x1,
        RENDERER_DX11 = 0x2,
        RENDERER_DX12 = 0x4,
        RENDERER_WARP_DX11 = RENDERER_WARP | RENDERER_DX11,
        RENDERER_WARP_DX12 = RENDERER_WARP | RENDERER_DX12
    };

    /// <summary>
    /// Initialize ninniku framework
    /// shaderPaths must point to compiled .cso folders
    /// </summary>
    NINNIKU_API bool Initialize(const ERenderer renderer, const std::vector<std::string_view>& shaderPaths, uint32_t flags = EInitializationFlags::IF_None, const ELogLevel logLevel = ELogLevel::LL_WARN_ERROR);

    /// <summary>
    /// Initialize ninniku framework
    /// alternative version where each shader blob must be manually loaded
    /// </summary>
    NINNIKU_API bool Initialize(const ERenderer renderer, uint32_t flags = EInitializationFlags::IF_None, const ELogLevel logLevel = ELogLevel::LL_WARN_ERROR);

    /// <summary>
    /// Cleanup resources used by ninniku
    /// If Renderdoc capture mode is enabled, finalize file capture
    /// </summary>
    NINNIKU_API void Terminate();
} // namespace ninniku
