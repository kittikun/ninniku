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
#include "core/renderer/null.h"
#include "utils/log.h"
#include "utils/misc.h"
#include "globals.h"

// for captures
#pragma comment(lib, "dxgi.lib")
#include <renderdoc/renderdoc_app.h>
#include <DXProgrammableCapture.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#include <Windows.h>

namespace ninniku
{
    Globals Globals::_instance;

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
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&Globals::Instance()._renderDocApi);

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
        return Globals::Instance()._renderer;
    }

    bool _Initialize(const ERenderer renderer)
    {
        if (Globals::Instance()._doCapture) {
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

        auto& dx = Globals::Instance()._renderer;

        switch (renderer) {
            case ERenderer::RENDERER_NULL:
            {
                dx.reset(new NullRenderer());
            }
            break;

            case ERenderer::RENDERER_DX11:
            case ERenderer::RENDERER_WARP_DX11:
            {
                dx.reset(new DX11(renderer));
            }
            break;

            case ERenderer::RENDERER_DX12:
            case ERenderer::RENDERER_WARP_DX12:
            {
                dx.reset(new DX12(renderer));
            }
            break;

            default:
                LOGE << "Unsupported renderer requested";
                return false;
        }

        if (!dx->Initialize()) {
            LOGE << "RenderDevice::Initialize failed";
            return false;
        }

        if (Globals::Instance()._doCapture) {
            // renderdoc doesn't support DXIL at the moment
            // https://renderdoc.org/docs/behind_scenes/d3d12_support.html#dxil-support
            if (((renderer & ERenderer::RENDERER_DX11) != 0) && ((renderer & ERenderer::RENDERER_WARP) == 0) && (Globals::Instance()._renderDocApi != nullptr)) {
                Globals::Instance()._renderDocApi->SetCaptureFilePathTemplate("ninniku");
                Globals::Instance()._renderDocApi->StartFrameCapture(nullptr, nullptr);
            }
        }

        return true;
    }

    void InitializeGlobals(uint32_t flags)
    {
        Globals::Instance()._doCapture = (flags & EInitializationFlags::IF_EnableCapture) != 0;
        Globals::Instance()._useDebugLayer = (flags & EInitializationFlags::IF_DisableDX12DebugLayer) == 0;
        Globals::Instance()._bc7Quick = (flags & EInitializationFlags::IF_BC7_QUICK_MODE) != 0;
    }

    void InitializeLog(const ELogLevel logLevel)
    {
        ninniku::Log::Initialize(logLevel);

        LOG << "ninniku HLSL compute shader framework";
    }

    void FixFlags(const ERenderer renderer, uint32_t& flags)
    {
        if (!IsDebuggerPresent() && ((renderer & ERenderer::RENDERER_DX12) != 0) && ((flags & EInitializationFlags::IF_DisableDX12DebugLayer) != 0)) {
            // disable is not debbugger is attached since you cannot see the messages anyway
            LOGW << "No debugger detected, disabling DX12 debug layer..";

            flags = flags & ~EInitializationFlags::IF_DisableDX12DebugLayer;
        }

        if (renderer == ERenderer::RENDERER_NULL)
            flags = 0;
    }

    bool Initialize(const ERenderer renderer, const std::vector<std::filesystem::path>& shaderPaths, uint32_t flags, const ELogLevel logLevel)
    {
        InitializeLog(logLevel);
        FixFlags(renderer, flags);
        InitializeGlobals(flags);

        if (!_Initialize(renderer))
            return false;

        if ((shaderPaths.size() > 0)) {
            auto& dx = GetRenderer();

            for (auto& path : shaderPaths) {
                if (!dx->LoadShader(path)) {
                    auto fmt = boost::format("Failed to load shaders in: %1%") % path;
                    LOGE << boost::str(fmt);
                    return false;
                }
            }
        }

        return true;
    }

    bool Initialize(const ERenderer renderer, uint32_t flags, const ELogLevel logLevel)
    {
        InitializeLog(logLevel);
        FixFlags(renderer, flags);
        InitializeGlobals(flags);

        return _Initialize(renderer);
    }

    void Terminate()
    {
        LOG << "Shutting down..";

        auto& renderer = Globals::Instance()._renderer;
        auto isNull = renderer->GetType() == ERenderer::RENDERER_NULL;

        if ((renderer->GetType() != ERenderer::RENDERER_NULL) && (Globals::Instance()._doCapture)) {
            if ((renderer->GetType() & ERenderer::RENDERER_DX11) != 0) {
                if (Globals::Instance()._renderDocApi != nullptr) {
                    Globals::Instance()._renderDocApi->EndFrameCapture(nullptr, nullptr);
                    Globals::Instance()._renderDocApi->Shutdown();
                    Globals::Instance()._renderDocApi = nullptr;
                }
            } else if ((renderer->GetType() & ERenderer::RENDERER_WARP) == 0) {
                Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> ga;
                if (!CheckAPIFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)), "DXGIGetDebugInterface1")) {
                    ga->EndCapture();
                }
            }
        }

        renderer->Finalize();
        renderer.reset();

        if (!isNull && Globals::Instance()._useDebugLayer) {
            Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
            if (!CheckAPIFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)), "DXGIGetDebugInterface1")) {
                LOG << "Reporting live objects";
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
        }

        LOG << "Shutdown complete";
    }
}