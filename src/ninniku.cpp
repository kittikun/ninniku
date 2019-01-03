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
#include "utils/misc.h"

#if defined(_USE_RENDERDOC)
#include <renderdoc/renderdoc_app.h>
#endif

namespace ninniku {
    static std::unique_ptr<DX11> sRenderer;

#if defined(_USE_RENDERDOC)
    RENDERDOC_API_1_1_2* gRenderDocApi = nullptr;
#endif

#if defined(_USE_RENDERDOC)
    void LoadRenderDoc()
    {
        LOG << "Loading RenderDoc..";

        std::string path = "renderdoc.dll";
        auto hInst = LoadLibrary(ninniku::strToWStr(path).c_str());

        if (hInst == nullptr) {
            auto fmt = boost::format("Failed to load %1%") % path;
            LOGE << boost::str(fmt);

            return;
        } else {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(hInst, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&gRenderDocApi);

            if (ret != 1) {
                LOGE << "Failed to get function pointer to RenderDoc API";
            }
        }
    }
#endif

    std::unique_ptr<DX11>& GetRenderer()
    {
        return sRenderer;
    }

    bool Initialize(uint8_t renderer, const std::string& shaderPath, uint8_t logLevel)
    {
        ninniku::Log::Initialize(logLevel);

        LOG << "ninniku HLSL compute shader framework";

#if defined(_USE_RENDERDOC)
        LoadRenderDoc();
#endif

        if ((renderer == RENDERER_DX11) || (renderer == RENDERER_WARP)) {
            sRenderer.reset(new ninniku::DX11());

            // since CI is running test, we must use warp driver
            if (!sRenderer->Initialize(shaderPath, (renderer == RENDERER_WARP) ? true : false)) {
                LOGE << "DX11App::Initialize failed";
                return false;
            }

#if defined(_USE_RENDERDOC)
            if (gRenderDocApi != nullptr) {
                gRenderDocApi->SetCaptureFilePathTemplate("ninniku");
                gRenderDocApi->StartFrameCapture(NULL, NULL);
            }
#endif

            return true;
        }

        return false;
    }

    void Terminate()
    {
#if defined(_USE_RENDERDOC)
        if (gRenderDocApi != nullptr)
            gRenderDocApi->EndFrameCapture(NULL, NULL);
#endif
    }
}