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
#include "ninniku/ninniku.h"

#include "ninniku/dx11/DX11.h"
#include "utils/log.h"

namespace ninniku
{
    static std::unique_ptr<DX11> sRenderer;

    std::unique_ptr<DX11>& GetRenderer()
    {
        return sRenderer;
    }

    bool Initialize(uint8_t renderer, const std::string& shaderPath, uint8_t logLevel)
    {
        ninniku::Log::Initialize(logLevel);

        LOG << "ninniku HLSL compute shader framework";

        if ((renderer == RENDERER_DX11) || (renderer == RENDERER_WARP)) {
            sRenderer.reset(new ninniku::DX11());

            // since CI is running test, we must use warp driver
            if (!sRenderer->Initialize(shaderPath, (renderer == RENDERER_WARP) ? true : false)) {
                LOGE << "DX11App::Initialize failed";
                return false;
            }

            return true;
        }

        return false;
    }
}