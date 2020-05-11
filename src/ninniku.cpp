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

// for captures
#pragma comment(lib, "dxgi.lib")
#include <renderdoc/renderdoc_app.h>
#include <DXProgrammableCapture.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>

namespace ninniku {
    static RENDERDOC_API_1_4_1* gRenderDocApi = nullptr;
    static RenderDeviceHandle sRenderer;
    static bool sDoCapture = false;

    void LoadRenderDoc()
    {
        LOG << "Loading RenderDoc..";

        std::string_view path = "renderdoc.dll";
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

    void LoadPIX()
    {
        LOG << "Loading PIX..";

        Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> ga;
        if (!CheckAPIFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)), "DXGIGetDebugInterface1")) {
            ga->BeginCapture();
        }
    }

    RenderDeviceHandle& GetRenderer()
    {
        return sRenderer;
    }

    bool _Initialize(const ERenderer renderer, const std::vector<std::string_view>& shaderPaths, uint32_t flags, const ELogLevel logLevel)
    {
        ninniku::Log::Initialize(logLevel);

        LOG << "ninniku HLSL compute shader framework";

        if (sDoCapture) {
            // renderdoc doesn't support DXIL at the moment
            // https://renderdoc.org/docs/behind_scenes/d3d12_support.html#dxil-support
            if ((renderer & ERenderer::RENDERER_DX11) != 0) {
                if ((renderer & ERenderer::RENDERER_WARP) == 0) {
                    LoadRenderDoc();
                } else {
                    LOGW << "Disabling Renderdoc for WARP devices";
                }
            } else {
                if ((renderer & ERenderer::RENDERER_WARP) == 0) {
                    LoadPIX();
                } else {
                    LOGW << "Disabling PIX for WARP devices";
                }
            }
        }

        switch (renderer) {
            case ERenderer::RENDERER_DX11:
            case ERenderer::RENDERER_WARP_DX11: {
                sRenderer.reset(new DX11(renderer));
            }
            break;

            case ERenderer::RENDERER_DX12:
            case ERenderer::RENDERER_WARP_DX12: {
                auto dx12 = new DX12(renderer);

                if ((flags & EInitializationFlags::IF_DisableDX12DebugLayer) == 0)
                    dx12->SetUseDebugLayer(true);

                sRenderer.reset(dx12);
            }
            break;

            default:
                LOGE << "Unsupported renderer requested";
                return false;
        }

        if (!sRenderer->Initialize(shaderPaths)) {
            LOGE << "RenderDevice::Initialize failed";
            return false;
        }

        if (sDoCapture) {
            // renderdoc doesn't support DXIL at the moment
            // https://renderdoc.org/docs/behind_scenes/d3d12_support.html#dxil-support
            if (((renderer & ERenderer::RENDERER_DX11) != 0) && ((renderer & ERenderer::RENDERER_WARP) == 0) && (gRenderDocApi != nullptr)) {
                gRenderDocApi->SetCaptureFilePathTemplate("ninniku");
                gRenderDocApi->StartFrameCapture(nullptr, nullptr);
            }
        }

        return true;
    }

    bool Initialize(const ERenderer renderer, const std::vector<std::string_view>& shaderPaths, uint32_t flags, const ELogLevel logLevel)
    {
        sDoCapture = (flags & EInitializationFlags::IF_EnableCapture) != 0;

        return _Initialize(renderer, shaderPaths, flags, logLevel);
    }

    bool Initialize(const ERenderer renderer, uint32_t flags, const ELogLevel logLevel)
    {
        std::vector<std::string_view> shaderPaths;

        sDoCapture = (flags & EInitializationFlags::IF_EnableCapture) != 0;

        return _Initialize(renderer, shaderPaths, flags, logLevel);
    }

    void Terminate()
    {
        LOG << "Shutting down..";

        if (sDoCapture) {
            if ((sRenderer->GetType() & ERenderer::RENDERER_DX11) != 0) {
                if (gRenderDocApi != nullptr) {
                    gRenderDocApi->EndFrameCapture(nullptr, nullptr);
                    gRenderDocApi->Shutdown();
                    gRenderDocApi = nullptr;
                }
            } else if ((sRenderer->GetType() & ERenderer::RENDERER_WARP) == 0) {
                Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> ga;
                if (!CheckAPIFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)), "DXGIGetDebugInterface1")) {
                    ga->EndCapture();
                }
            }
        }

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