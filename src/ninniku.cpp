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

#include "pch.h"
#include "ninniku/ninniku.h"

#include "ninniku/core/renderer/renderdevice.h"
#include "core/renderer/dx11/DX11.h"
#include "core/renderer/dx12/DX12.h"
#include "utils/log.h"
#include "utils/misc.h"

#if defined(_USE_RENDERDOC)
#include <renderdoc/renderdoc_app.h>
#endif

#ifdef _DEBUG
#pragma comment(lib, "dxgi.lib")

#include <dxgi1_4.h>
#include <dxgidebug.h>
#endif

namespace ninniku {
#if defined(_USE_RENDERDOC)
    RENDERDOC_API_1_4_1* gRenderDocApi = nullptr;
#endif

    static RenderDeviceHandle sRenderer;

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
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&gRenderDocApi);

            if (ret != 1) {
                LOGE << "Failed to get function pointer to RenderDoc API";
            }
        }
    }
#endif

    RenderDeviceHandle& GetRenderer()
    {
        return sRenderer;
    }

    bool _Initialize(const ERenderer renderer, const std::vector<std::string>& shaderPaths, const ELogLevel logLevel)
    {
        ninniku::Log::Initialize(logLevel);

        LOG << "ninniku HLSL compute shader framework";

#if defined(_USE_RENDERDOC)
        // renderdoc doesn't support DXIL at the moment
        // https://renderdoc.org/docs/behind_scenes/d3d12_support.html#dxil-support
        if ((renderer & ERenderer::RENDERER_DX12) == 0) {
            LoadRenderDoc();
        } else {
            LOGW << "Renderdoc doesn't support DXIL so disabling it";
        }
#endif

        switch (renderer) {
            case ERenderer::RENDERER_DX11:
            case ERenderer::RENDERER_WARP_DX11: {
                sRenderer.reset(new DX11(renderer));
            }
            break;

            case ERenderer::RENDERER_DX12:
            case ERenderer::RENDERER_WARP_DX12: {
                sRenderer.reset(new DX12(renderer));
            }
            break;

            default:
                LOGE << "Unsupported renderer requested";
                return false;
        }

        // since CI is running test, we must use warp driver
        if (!sRenderer->Initialize(shaderPaths)) {
            LOGE << "RenderDevice::Initialize failed";
            return false;
        }

#if defined(_USE_RENDERDOC)
        if (gRenderDocApi != nullptr) {
            gRenderDocApi->SetCaptureFilePathTemplate("ninniku");
            gRenderDocApi->StartFrameCapture(nullptr, nullptr);
        }
#endif

        return true;
    }

    bool Initialize(const ERenderer renderer, const std::vector<std::string>& shaderPaths, const ELogLevel logLevel)
    {
        return _Initialize(renderer, shaderPaths, logLevel);
    }

    bool Initialize(const ERenderer renderer, const ELogLevel logLevel)
    {
        std::vector<std::string> shaderPaths;

        return _Initialize(renderer, shaderPaths, logLevel);
    }

    void Terminate()
    {
        LOG << "Shutting down..";

#if defined(_USE_RENDERDOC)
        if (gRenderDocApi != nullptr) {
            gRenderDocApi->EndFrameCapture(nullptr, nullptr);
            gRenderDocApi->Shutdown();
            gRenderDocApi = nullptr;
        }
#endif

        sRenderer->Finalize();
        sRenderer.reset();

#ifdef _DEBUG
        Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
        if (!CheckAPIFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)), "DXGIGetDebugInterface1")) {
            LOG << "Reporting live objects";
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
#endif

        LOG << "Shutdown complete";
    }
}