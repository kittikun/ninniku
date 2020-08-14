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

#include "../../utils/misc.h"

#include "dxgi.h"

namespace ninniku
{
    static Microsoft::WRL::ComPtr<IDXGIFactory5> DXGIFactory;

    IDXGIFactory5* DXGI::GetDXGIFactory5()
    {
        if (!DXGIFactory) {
            typedef HRESULT(WINAPI* pfn_CreateDXGIFactory2)(UINT Flags, REFIID riid, _Out_ void** ppFactory);

            static pfn_CreateDXGIFactory2 s_CreateDXGIFactory2 = nullptr;

            if (!s_CreateDXGIFactory2) {
                auto hModDXGI = LoadLibrary(L"dxgi.dll");

                if (!hModDXGI) {
                    LOGE << "Failed to get load dxgi.dll";
                    return nullptr;
                }

                s_CreateDXGIFactory2 = reinterpret_cast<pfn_CreateDXGIFactory2>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory2")));

                if (!s_CreateDXGIFactory2) {
                    LOGE << "Failed to get function pointer to CreateDXGIFactory2";
                    return nullptr;
                }
            }

            uint32_t flags = 0;

#ifdef _DEBUG
            flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

            if (CheckAPIFailed(s_CreateDXGIFactory2(flags, IID_PPV_ARGS(&DXGIFactory)), "Failed to CreateDXGIFactory2 "))
                return nullptr;
        }

        return DXGIFactory.Get();
    }

    bool DXGI::CreateSwapchain(IUnknown* device, const SwapchainParam& param, DXGISwapChain& dst)
    {
        // this will happen for DX11 devices
        if (DXGIFactory == nullptr)
            GetDXGIFactory5();

        if (DXGIFactory != nullptr) {
            DXGI_SWAP_CHAIN_DESC1 scDesc = {};

            scDesc.Width = param.width;
            scDesc.Height = param.height;
            scDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(param.format));
            scDesc.SampleDesc.Count = 1;
            scDesc.SampleDesc.Quality = 0;
            scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scDesc.BufferCount = param.bufferCount;
            scDesc.Scaling = DXGI_SCALING_NONE;
            scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;

            auto hr = DXGIFactory->CreateSwapChainForHwnd(device, param.hwnd, &scDesc, nullptr, nullptr, &swapChain);

            if (CheckAPIFailed(hr, "Failed to create swap chain"))
                return false;

            swapChain.As(&dst);
        } else {
            LOGE << "Failed to create IDXGIFactory5";
            return false;
        }

        return true;
    }

    void DXGI::ReleaseDXGIFactory()
    {
        DXGIFactory.Reset();
    }
} // namespace ninniku