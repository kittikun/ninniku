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
#include "DX12.h"

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d12.lib")

#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../DXCommon.h"

#pragma warning(push)
#pragma warning(disable:4100)
#include "pix3.h"
#pragma warning(pop)

#include <comdef.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#include <dxc/dxcapi.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/Support/d3dx12.h>

namespace ninniku {
    void DX12::CopyBufferResource([[maybe_unused]]const CopyBufferSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresource([[maybe_unused]]const CopyTextureSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    BufferHandle DX12::CreateBuffer(const BufferParamHandle& params)
    {
        auto isSRV = (params->viewflags & EResourceViews::RV_SRV) != 0;
        auto isUAV = (params->viewflags & EResourceViews::RV_UAV) != 0;
        auto bufferSize = params->numElements * params->elementSize;

        auto fmt = boost::format("Creating Buffer: ElementSize=%1%, NumElements=%2%, Size=%3%") % params->elementSize % params->numElements % bufferSize;
        LOGD << boost::str(fmt);

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if (isUAV)
            flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        auto res = std::make_unique<DX12BufferObject>();

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

        auto hr = _device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(res->_buffer.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return BufferHandle();

        if (isSRV) {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = params->numElements;
            srvDesc.Buffer.StructureByteStride = params->elementSize;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            auto srv = new DX12ShaderResourceView();

            srv->_srvUAVIndex = _srvUAVIndex++;

            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(_srvUAVHeap->GetCPUDescriptorHandleForHeapStart(), srv->_srvUAVIndex, _srvUAVDescriptorSize);

            _device->CreateShaderResourceView(res->_buffer.Get(), &srvDesc, srvHandle);

            res->_srv.reset(srv);
        }

        if (isUAV) {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = 0;
            uavDesc.Buffer.NumElements = params->numElements;
            uavDesc.Buffer.StructureByteStride = params->elementSize;
            uavDesc.Buffer.CounterOffsetInBytes = 0;
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            auto uav = new DX12UnorderedAccessView();

            uav->_srvUAVIndex = _srvUAVIndex++;

            CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle{ _srvUAVHeap->GetCPUDescriptorHandleForHeapStart(), uav->_srvUAVIndex, _srvUAVDescriptorSize };

            _device->CreateUnorderedAccessView(res->_buffer.Get(), nullptr, &uavDesc, uavHandle);

            uav->_resource = res->_buffer;
            res->_uav.reset(uav);
        }

        return res;
    }

    BufferHandle DX12::CreateBuffer([[maybe_unused]]const BufferHandle& src)
    {
        throw std::exception("not implemented");
    }

    DebugMarkerHandle DX12::CreateDebugMarker(const std::string& name) const
    {
        return std::make_unique<DX12DebugMarker>(name);
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

        Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;

        // if an exception if thrown here, you might need to install the graphics tools
        // https://msdn.microsoft.com/en-us/library/mt125501.aspx
        if (SUCCEEDED(s_DynamicD3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
            debugInterface->EnableDebugLayer();

            Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
            if (SUCCEEDED(debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1)))) {
                debugInterface1->SetEnableGPUBasedValidation(true);
            } else {
                LOGE << "CreateDevice failed to get ID3D12Debug1";
                return false;
            }
        } else {
            LOGE << "CreateDevice failed to get ID3D12Debug";
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

    TextureHandle DX12::CreateTexture([[maybe_unused]]const TextureParamHandle& params)
    {
        throw std::exception("not implemented");
    }

    bool DX12::Dispatch(const CommandHandle& cmd) const
    {
        DX12Command* dxCmd = static_cast<DX12Command*>(cmd.get());

        if (!dxCmd->_isInitialized) {
            auto foundShader = _shaders.find(cmd->shader);

            if (foundShader == _shaders.end()) {
                auto fmt = boost::format("Dispatch error: could not find shader \"%1%\"") % cmd->shader;
                LOGE << boost::str(fmt);
                return false;
            }

            auto foundRS = _rootSignatures.find(cmd->shader);

            if (foundRS == _rootSignatures.end()) {
                auto fmt = boost::format("Dispatch error: could not find the root signature for shader \"%1%\"") % cmd->shader;
                LOGE << boost::str(fmt);
                return false;
            }

            if (!dxCmd->Initialize(_device, _commandAllocator, foundShader->second, foundRS->second)) {
                return false;
            }
        }

        dxCmd->_cmdList->SetPipelineState(dxCmd->_pso.Get());
        dxCmd->_cmdList->SetComputeRootSignature(dxCmd->_rootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { _srvUAVHeap.Get() };
        dxCmd->_cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        // resource bindings for this shader
        auto foundBindings = _resourceBindings.find(cmd->shader);

        if (foundBindings == _resourceBindings.end()) {
            auto fmt = boost::format("Dispatch error: could not find resource bindings for shader \"%1%\"") % cmd->shader;
            LOGE << boost::str(fmt);
            return false;
        }

        auto& bindings = foundBindings->second;

        for (auto& kvp : cmd->uavBindings) {
            auto found = bindings.find(kvp.first);

            if (found == bindings.end()) {
                auto fmt = boost::format("Dispatch error: could not find resource bindings for \"%1%\" in \"%2%\"") % kvp.first % cmd->shader;
                LOGE << boost::str(fmt);
                return false;
            }

            auto dxUAV = static_cast<const DX12UnorderedAccessView*>(kvp.second);

            CD3DX12_RESOURCE_BARRIER pushBarrier = CD3DX12_RESOURCE_BARRIER::Transition(dxUAV->_resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            dxCmd->_cmdList->ResourceBarrier(1, &pushBarrier);

            CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle{ _srvUAVHeap->GetGPUDescriptorHandleForHeapStart(), dxUAV->_srvUAVIndex, _srvUAVDescriptorSize };
            dxCmd->_cmdList->SetComputeRootDescriptorTable(0, uavHandle);
            dxCmd->_cmdList->Dispatch(cmd->dispatch[0], cmd->dispatch[1], cmd->dispatch[2]);

            CD3DX12_RESOURCE_BARRIER popBarrier = CD3DX12_RESOURCE_BARRIER::Transition(dxUAV->_resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            dxCmd->_cmdList->ResourceBarrier(1, &popBarrier);
        }

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

        if (!InitializeHeaps())
            return false;

        // Create command allocator (only for compute for now)
        auto hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&_commandAllocator));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator"))
            return false;

        return true;
    }

    bool DX12::InitializeHeaps()
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
        srvUavHeapDesc.NumDescriptors = MAX_DESCRIPTOR_COUNT;
        srvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr;

        hr = _device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&_srvUAVHeap));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap"))
            return false;

        _srvUAVDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        return true;
    }

    bool DX12::LoadShader([[maybe_unused]]const std::string& name, [[maybe_unused]]const void* pData, [[maybe_unused]]const size_t size)
    {
        throw std::exception("not implemented");
    }

    bool DX12::LoadShader(const std::string& name, IDxcBlobEncoding* pBlob)
    {
        Microsoft::WRL::ComPtr<IDxcContainerReflection> pContainerReflection;

        auto hr = DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)&pContainerReflection);

        if (CheckAPIFailed(hr, "DxcCreateInstance for CLSID_DxcContainerReflection"))
            return false;

        hr = pContainerReflection->Load(pBlob);

        if (CheckAPIFailed(hr, "IDxcContainerReflection::Load"))
            return false;

        uint32_t partCount;

        hr = pContainerReflection->GetPartCount(&partCount);

        if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartCount"))
            return false;

        for (uint32_t i = 0; i < partCount; ++i) {
            uint32_t partKind;

            hr = pContainerReflection->GetPartKind(i, &partKind);

            if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartKind"))
                return false;

            if (partKind == static_cast<uint32_t>(hlsl::DxilFourCC::DFCC_DXIL)) {
#if _DEBUG
                IDxcBlob* pShaderBlob;
                hr = pContainerReflection->GetPartContent(i, &pShaderBlob);

                if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartContent"))
                    return false;

                auto pHeader = reinterpret_cast<const hlsl::DxilProgramHeader*>(pShaderBlob->GetBufferPointer());

                // we can probably afford to only run this in debug
                if (!IsValidDxilProgramHeader(pHeader, static_cast<uint32_t>(pShaderBlob->GetBufferSize()))) {
                    LOGE << "DXIL program header is invalid";
                    return false;
                }
#endif

                Microsoft::WRL::ComPtr<ID3D12ShaderReflection> pShaderReflection;

                hr = pContainerReflection->GetPartReflection(i, IID_PPV_ARGS(&pShaderReflection));

                if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartReflection"))
                    return false;

                D3D12_SHADER_DESC pShaderDesc;
                hr = pShaderReflection->GetDesc(&pShaderDesc);

                if (CheckAPIFailed(hr, "ID3D12ShaderReflection::GetDesc"))
                    return false;

                if (!ParseShaderResources(name, pShaderDesc.BoundResources, pShaderReflection.Get()))
                    return false;
            } else if (partKind == static_cast<uint32_t>(hlsl::DxilFourCC::DFCC_RootSignature)) {
#if _DEBUG
                // not really necessary but as an extra precaution
                Microsoft::WRL::ComPtr<IDxcBlob> pRSBlob;
                hr = pContainerReflection->GetPartContent(i, &pRSBlob);

                if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartContent"))
                    return false;
#endif
                if (!ParseRootSignature(name, pBlob))
                    return false;
            }
        }

        // store shader
        _shaders.insert(std::make_pair(name, CD3DX12_SHADER_BYTECODE(pBlob->GetBufferPointer(), pBlob->GetBufferSize())));

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

        if (CheckAPIFailed(hr, "DxcCreateInstance for CLSID_DxcLibrary"))
            return false;

        for (auto& iter : std::filesystem::recursive_directory_iterator(shaderPath)) {
            if (iter.path().extension() == ext) {
                auto path = iter.path().string();

                fmt = boost::format("Loading %1%..") % path;

                LOG_INDENT_START << boost::str(fmt);

                Microsoft::WRL::ComPtr<IDxcBlobEncoding> pBlob = nullptr;

                hr = pLibrary->CreateBlobFromFile(ninniku::strToWStr(path).c_str(), nullptr, &pBlob);

                if (CheckAPIFailed(hr, "IDxcLibrary::CreateBlobFromFile"))
                    return false;

                auto name = iter.path().stem().string();

                if (!LoadShader(name, pBlob.Get()))
                    return false;
            }
        }

        return true;
    }

    MappedResourceHandle DX12::MapBuffer([[maybe_unused]]const BufferHandle& bObj)
    {
        throw std::exception("not implemented");
    }

    MappedResourceHandle DX12::MapTexture([[maybe_unused]]const TextureHandle& tObj, [[maybe_unused]]const uint32_t index)
    {
        throw std::exception("not implemented");
    }

    bool DX12::ParseRootSignature(const std::string& name, IDxcBlobEncoding* pBlob)
    {
        DX12RootSignature rootSignature;

        auto hr = _device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateRootSignature"))
            return false;

        LOGD << "Found a root signature";

        // for now create a root signature per shader
        _rootSignatures.insert(std::make_pair(name, rootSignature));

        return true;
    }

    bool DX12::ParseShaderResources(const std::string& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection)
    {
        // parse parameter bind slots
        auto fmt = boost::format("Found %1% resources") % numBoundResources;

        LOGD_INDENT_START << boost::str(fmt);

        std::unordered_map<std::string, uint32_t> bindings;

        for (uint32_t i = 0; i < numBoundResources; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;

            auto hr = pReflection->GetResourceBindingDesc(i, &bindDesc);

            if (CheckAPIFailed(hr, "ID3D12ShaderReflection::GetResourceBindingDesc"))
                return false;

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
                    LOG << "DX12::ParseShaderResources unsupported type";
                    return false;
            }

            fmt = boost::format("Resource: Name=\"%1%\", Type=%2%, Slot=%3%") % bindDesc.Name % restypeStr % bindDesc.BindPoint;

            LOGD << boost::str(fmt);

            bindings.insert(std::make_pair(bindDesc.Name, bindDesc.BindPoint));
        }

        _resourceBindings.insert(std::make_pair(name, bindings));

        LOGD_INDENT_END;

        return true;
    }

    bool DX12::UpdateConstantBuffer([[maybe_unused]]const std::string& name, [[maybe_unused]]void* data, [[maybe_unused]]const uint32_t size)
    {
        throw std::exception("not implemented");
    }
} // namespace ninniku