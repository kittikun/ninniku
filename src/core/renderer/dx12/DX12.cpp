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
#include "DX12.h"

#include "../../../utils/log.h"
#include "../DXCommon.h"

#include <dxgi1_3.h>

namespace ninniku
{
    void DX12::CopyBufferResource(const CopyBufferSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresource(const CopyTextureSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    BufferHandle DX12::CreateBuffer(const BufferParamHandle& params)
    {
        throw std::exception("not implemented");
    }

    BufferHandle DX12::CreateBuffer(const BufferHandle& src)
    {
        throw std::exception("not implemented");
    }

    CommandHandle DX12::CreateCommand() const
    {
        throw std::exception("not implemented");
    }

    DebugMarkerHandle DX12::CreateDebugMarker(const std::string& name) const
    {
        throw std::exception("not implemented");
    }

    bool DX12::CreateDevice(int adapter)
    {
        LOGD << "Creating ID3D12Device..";

#if defined(_DEBUG)
        static PFN_D3D12_GET_DEBUG_INTERFACE s_DynamicD3D12GetDebugInterface = nullptr;

        if (!s_DynamicD3D12GetDebugInterface) {
            HMODULE hModD3D12 = LoadLibrary(L"d3d12.dll");
            if (!hModD3D12)
                return false;

            s_DynamicD3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12GetDebugInterface")));
            if (!s_DynamicD3D12GetDebugInterface)
                return false;
        }

        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

        // if an exception if thrown here, you might need to install the graphics tools
        // https://msdn.microsoft.com/en-us/library/mt125501.aspx
        if (SUCCEEDED(s_DynamicD3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
        } else {
            LOGE << "CreateDevice failed to get the debug controller";
            return false;
        }
#endif

        static PFN_D3D12_CREATE_DEVICE s_DynamicD3D12CreateDevice = nullptr;

        if (!s_DynamicD3D12CreateDevice) {
            HMODULE hModD3D12 = LoadLibrary(L"d3d12.dll");
            if (!hModD3D12)
                return false;

            s_DynamicD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12CreateDevice")));
            if (!s_DynamicD3D12CreateDevice)
                return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
        D3D_DRIVER_TYPE driverType;

        if (adapter >= 0) {
            Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;

            if (DXCommon::GetDXGIFactory(dxgiFactory.GetAddressOf())) {
                if (FAILED(dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf()))) {
                    auto fmt = boost::format("Invalid GPU adapter index (%1%)!") % adapter;
                    LOGE << boost::str(fmt);
                    return false;
                }
            }

            driverType = (pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
        } else {
            driverType = D3D_DRIVER_TYPE_WARP;
        }

        auto minFeatureLevel = D3D_FEATURE_LEVEL_12_0;

        auto hr = s_DynamicD3D12CreateDevice(pAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&_device));

        Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;

        if (SUCCEEDED(hr)) {
            DXGI_ADAPTER_DESC desc;
            hr = pAdapter->GetDesc(&desc);

            if (SUCCEEDED(hr)) {
                auto fmt = boost::wformat(L"Using DirectCompute on %1%") % desc.Description;
                LOGD << boost::str(fmt);
                return true;
            }
        }

        return false;
    }

    TextureHandle DX12::CreateTexture(const TextureParamHandle& params)
    {
        throw std::exception("not implemented");
    }

    bool DX12::Dispatch(const CommandHandle& cmd) const
    {
        throw std::exception("not implemented");
    }

    bool DX12::Initialize(const std::vector<std::string>& shaderPaths, const bool isWarp)
    {
        auto adapter = 0;

        if (isWarp)
            adapter = -1;

        if (!CreateDevice(adapter)) {
            LOGE << "Failed to create DX12 device";
            return false;
        }

        return true;
    }

    bool DX12::LoadShader(const std::string& name, const void* pData, const size_t size)
    {
        throw std::exception("not implemented");
    }

    MappedResourceHandle DX12::MapBuffer(const BufferHandle& bObj)
    {
        throw std::exception("not implemented");
    }

    MappedResourceHandle DX12::MapTexture(const TextureHandle& tObj, const uint32_t index)
    {
        throw std::exception("not implemented");
    }

    bool DX12::UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size)
    {
        throw std::exception("not implemented");
    }
} // namespace ninniku