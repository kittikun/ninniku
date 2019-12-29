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

#pragma comment(lib, "dxcompiler.lib")

#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../DXCommon.h"

#include <comdef.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#include <dxc/dxcapi.h>
#include <dxc/DxilContainer/DxilContainer.h>

namespace ninniku {
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

        auto hModD3D12 = LoadLibrary(L"d3d12.dll");

        if (!hModD3D12)
            return false;

#if defined(_DEBUG)
        static PFN_D3D12_GET_DEBUG_INTERFACE s_DynamicD3D12GetDebugInterface = nullptr;

        if (!s_DynamicD3D12GetDebugInterface) {
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
            s_DynamicD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12CreateDevice")));
            if (!s_DynamicD3D12CreateDevice)
                return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
        D3D_DRIVER_TYPE driverType;

        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;

        if (DXCommon::GetDXGIFactory<IDXGIFactory4>(dxgiFactory.GetAddressOf())) {
            if (adapter < 0) {
                // WARP
                if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)))) {
                    LOGE << "Failed to Enumerate WARP adapter";
                    return false;
                }

                driverType = D3D_DRIVER_TYPE_WARP;
            } else {
                if (FAILED(dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf()))) {
                    // HW
                    auto fmt = boost::format("Invalid GPU adapter index (%1%)!") % adapter;
                    LOGE << boost::str(fmt);
                    return false;
                }

                driverType = (pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
            }
        } else {
            LOGE << "Failed to to create IDXGIFactory4";
            return false;
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

        if ((shaderPaths.size() > 0)) {
            for (auto& path : shaderPaths) {
                if (!LoadShaders(path)) {
                    auto fmt = boost::format("Failed to load shaders in: %1%") % path;
                    LOGE << boost::str(fmt);
                    return false;
                }
            }
        }

        return true;
    }

    bool DX12::LoadShader(const std::string& name, const void* pData, const size_t size)
    {
        throw std::exception("not implemented");
    }

    bool DX12::LoadShader(const std::string& name, IDxcBlobEncoding* pBlob, const std::string& path)
    {
        Microsoft::WRL::ComPtr<IDxcContainerReflection> pContainerReflection;

        auto hr = DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)&pContainerReflection);

        if (FAILED(hr)) {
            LOGE << "DxcCreateInstance for CLSID_DxcContainerReflection failed with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        hr = pContainerReflection->Load(pBlob);

        if (FAILED(hr)) {
            LOGE << "IDxcContainerReflection::Load failed with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        uint32_t partCount;

        hr = pContainerReflection->GetPartCount(&partCount);

        if (FAILED(hr)) {
            LOGE << "IDxcContainerReflection::GetPartCount failed with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        for (uint32_t i = 0; i < partCount; ++i) {
            uint32_t partKind;

            hr = pContainerReflection->GetPartKind(i, &partKind);

            if (FAILED(hr)) {
                LOGE << "IDxcContainerReflection::GetPartKind failed with:";
                _com_error err(hr);
                LOGE << err.ErrorMessage();
                return false;
            }

            if (partKind == static_cast<uint32_t>(hlsl::DxilFourCC::DFCC_DXIL)) {
                IDxcBlob* pBlob;
                hr = pContainerReflection->GetPartContent(i, &pBlob);

                if (FAILED(hr)) {
                    LOGE << "IDxcContainerReflection::GetPartContent failed with:";
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                auto pHeader = reinterpret_cast<const hlsl::DxilProgramHeader*>(pBlob->GetBufferPointer());

#if _DEBUG
                // we can probably afford to only run this in debug
                if (!IsValidDxilProgramHeader(pHeader, static_cast<uint32_t>(pBlob->GetBufferSize()))) {
                    LOGE << "DXIL program header is invalid";
                    return false;
                }
#endif

                auto shaderKind = hlsl::GetVersionShaderType(pHeader->ProgramVersion);

                Microsoft::WRL::ComPtr<ID3D12ShaderReflection> pShaderReflection;

                hr = pContainerReflection->GetPartReflection(i, IID_PPV_ARGS(&pShaderReflection));

                if (FAILED(hr)) {
                    LOGE << "IDxcContainerReflection::GetPartReflection failed with:";
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                D3D12_SHADER_DESC pShaderDesc;
                hr = pShaderReflection->GetDesc(&pShaderDesc);

                if (FAILED(hr)) {
                    LOGE << "ID3D12ShaderReflection::GetDesc failed with:";
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                ParseShaderResources(pShaderDesc.BoundResources, pShaderReflection.Get());
            }
        }

        return true;
    }

    /// <summary>
    /// Load all shaders in /data
    /// </summary>
    bool DX12::LoadShaders(const std::string& shaderPath)
    {
        // check if directory is valid
        if (!std::filesystem::is_directory(shaderPath)) {
            auto fmt = boost::format("Failed to open directory: %1%") % shaderPath;
            LOGE << boost::str(fmt);

            return false;
        }

        std::string ext{ ".dxco" };

        // Count the number of .cso found
        std::filesystem::directory_iterator begin(shaderPath), end;

        auto fileCounter = [&](const std::filesystem::directory_entry & d) {
            return (!is_directory(d.path()) && (d.path().extension() == ext));
        };

        auto numFiles = std::count_if(begin, end, fileCounter);
        auto fmt = boost::format("Found %1% compiled shaders in /%2%") % numFiles % shaderPath;

        LOGD << boost::str(fmt);

        IDxcLibrary* pLibrary = nullptr;

        auto hr = DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary);

        if (FAILED(hr)) {
            LOGE << "DxcCreateInstance for CLSID_DxcLibrary failed with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        for (auto& iter : std::filesystem::recursive_directory_iterator(shaderPath)) {
            if (iter.path().extension() == ext) {
                auto path = iter.path().string();

                fmt = boost::format("Loading %1%..") % path;

                LOG_INDENT_START << boost::str(fmt);

                Microsoft::WRL::ComPtr<IDxcBlobEncoding> pBlob = nullptr;

                hr = pLibrary->CreateBlobFromFile(ninniku::strToWStr(path).c_str(), nullptr, &pBlob);

                if (FAILED(hr)) {
                    LOGE << "Failed to IDxcLibrary::CreateBlobFromFile with:";
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                auto name = iter.path().stem().string();

                if (!LoadShader(name, pBlob.Get(), path))
                    return false;
            }
        }

        return true;
    }

    MappedResourceHandle DX12::MapBuffer(const BufferHandle& bObj)
    {
        throw std::exception("not implemented");
    }

    MappedResourceHandle DX12::MapTexture(const TextureHandle& tObj, const uint32_t index)
    {
        throw std::exception("not implemented");
    }

    std::unordered_map<std::string, uint32_t> DX12::ParseShaderResources(uint32_t numBoundResources, ID3D12ShaderReflection* pReflection)
    {
        std::unordered_map<std::string, uint32_t> res;

        // parse parameter bind slots
        for (uint32_t i = 0; i < numBoundResources; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;

            auto hr = pReflection->GetResourceBindingDesc(i, &bindDesc);

            if (FAILED(hr)) {
                LOGE << "Failed to ID3D12ShaderReflection::GetResourceBindingDesc with:";
                _com_error err(hr);
                LOGE << err.ErrorMessage();
                return res;
            }

            std::string restypeStr;

            switch (bindDesc.Type) {
                case D3D_SIT_CBUFFER:
                    restypeStr = "D3D_SIT_SAMPLER";
                    break;

                case D3D_SIT_SAMPLER:
                    restypeStr = "D3D_SIT_SAMPLER";
                    break;

                case D3D_SIT_STRUCTURED:
                    restypeStr = "D3D_SIT_STRUCTURED";
                    break;

                case D3D_SIT_TEXTURE:
                    restypeStr = "D3D_SIT_TEXTURE";
                    break;

                case D3D_SIT_UAV_RWTYPED:
                    restypeStr = "D3D_SIT_UAV_RWTYPED";
                    break;

                case D3D_SIT_UAV_RWSTRUCTURED:
                    restypeStr = "D3D_SIT_UAV_RWSTRUCTURED";
                    break;

                default:
                    throw new std::exception("DX12::ParseShaderResources unsupported type");
            }

            auto fmt = boost::format("Resource: Name=\"%1%\", Type=%2%, Slot=%3%") % bindDesc.Name % restypeStr % bindDesc.BindPoint;

            LOGD << boost::str(fmt);

            res.insert(std::make_pair(bindDesc.Name, bindDesc.BindPoint));
        }

        return res;
    }

    bool DX12::UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size)
    {
        throw std::exception("not implemented");
    }
} // namespace ninniku