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
#pragma comment(lib, "dxgi.lib")

#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../../../utils/objectTracker.h"
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
    DX12::DX12(ERenderer type)
        : _type{ type }
    {
    }

    void DX12::CopyBufferResource(const CopyBufferSubresourceParam& params)
    {
        auto srcImpl = static_cast<const DX12BufferImpl*>(params.src);
        auto srcInternal = srcImpl->_impl.lock();

        auto dstImpl = static_cast<const DX12BufferImpl*>(params.dst);
        auto dstInternal = dstImpl->_impl.lock();

        CD3DX12_RESOURCE_BARRIER pushBarrierSrc = CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

        // we assume that dst is already in D3D12_RESOURCE_STATE_COPY_DEST
        _copyCmdList->ResourceBarrier(1, &pushBarrierSrc);
        _copyCmdList->CopyResource(dstInternal->_buffer.Get(), srcInternal->_buffer.Get());

        CD3DX12_RESOURCE_BARRIER popBarrierSrc = CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_buffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);

        _copyCmdList->ResourceBarrier(1, &popBarrierSrc);

        _copyCmdList->Close();

        ExecuteCommand(_copyCmdList);

        _copyCmdList->Reset(_commandAllocatorCopy.Get(), nullptr);
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresource([[maybe_unused]]const CopyTextureSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    BufferHandle DX12::CreateBuffer(const BufferParamHandle& params)
    {
        auto isSRV = (params->viewflags & EResourceViews::RV_SRV) != 0;
        auto isUAV = (params->viewflags & EResourceViews::RV_UAV) != 0;
        auto isCPURead = (params->viewflags & EResourceViews::RV_CPU_READ) != 0;
        auto bufferSize = params->numElements * params->elementSize;

        auto fmt = boost::format("Creating Buffer: ElementSize=%1%, NumElements=%2%, Size=%3%") % params->elementSize % params->numElements % bufferSize;
        LOGD << boost::str(fmt);

        auto impl = std::make_shared<DX12BufferInternal>();

        ObjectTracker::Instance().RegisterObject(impl);

        impl->_desc = params;

        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_RESOURCE_STATES resState = D3D12_RESOURCE_STATE_COMMON;
        D3D12_HEAP_TYPE heapFlags = D3D12_HEAP_TYPE_DEFAULT;

        if (isUAV)
            resFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if (isCPURead) {
            resState = D3D12_RESOURCE_STATE_COPY_DEST;
            heapFlags = D3D12_HEAP_TYPE_READBACK;
        }

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(heapFlags);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resFlags);

        auto hr = _device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, resState, nullptr, IID_PPV_ARGS(impl->_buffer.GetAddressOf()));

        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            hr = _device->GetDeviceRemovedReason();

            CheckAPIFailed(hr, "ID3D12Device::GetDeviceRemovedReason");
        }

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

            srv->_resource = impl->_buffer;

            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(_srvUAVHeap->GetCPUDescriptorHandleForHeapStart(), _srvUAVIndex++, _srvUAVDescriptorSize);

            _device->CreateShaderResourceView(impl->_buffer.Get(), &srvDesc, srvHandle);

            impl->_srv.reset(srv);
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

            uav->_resource = impl->_buffer;

            CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle{ _srvUAVHeap->GetCPUDescriptorHandleForHeapStart(), _srvUAVIndex++, _srvUAVDescriptorSize };

            _device->CreateUnorderedAccessView(impl->_buffer.Get(), nullptr, &uavDesc, uavHandle);

            uav->_resource = impl->_buffer;
            impl->_uav.reset(uav);
        }

        return std::make_unique<DX12BufferImpl>(impl);
    }

    BufferHandle DX12::CreateBuffer(const BufferHandle& src)
    {
        auto implSrc = static_cast<const DX12BufferImpl*>(src.get());
        auto internalSrc = implSrc->_impl.lock();

        assert(internalSrc->_desc->elementSize % 4 == 0);

        auto marker = CreateDebugMarker("CreateBufferFromBufferObject");

        auto dst = CreateBuffer(internalSrc->_desc);
        auto implDst = static_cast<const DX12BufferImpl*>(dst.get());
        auto internalDst = implDst->_impl.lock();

        // copy src to dst
        {
            CopyBufferSubresourceParam copyParams = {};

            copyParams.src = src.get();
            copyParams.dst = dst.get();

            CopyBufferResource(copyParams);
        }

        // create a temporary object readable from CPU to fill internalDst->_data with a map
        auto stride = internalSrc->_desc->elementSize / 4;
        auto params = internalSrc->_desc->Duplicate();

        // allocate memory
        internalDst->_data.resize(stride * internalSrc->_desc->numElements);
        params->viewflags = RV_CPU_READ;

        auto temp = CreateBuffer(params);

        // copy src to temp
        {
            CopyBufferSubresourceParam copyParams = {};

            copyParams.src = src.get();
            copyParams.dst = temp.get();

            CopyBufferResource(copyParams);
        }

        auto mapped = Map(temp);
        uint32_t dstPitch = static_cast<uint32_t>(internalDst->_data.size() * sizeof(uint32_t));

        memcpy_s(&internalDst->_data.front(), dstPitch, mapped->GetData(), dstPitch);

        return dst;
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

        static PFN_D3D12_GET_DEBUG_INTERFACE s_DynamicD3D12GetDebugInterface = nullptr;

        if (!s_DynamicD3D12GetDebugInterface) {
            s_DynamicD3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12GetDebugInterface")));
            if (!s_DynamicD3D12GetDebugInterface)
                return false;
        }

        Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;

        // if an exception if thrown here, you might need to install the graphics tools
        // https://msdn.microsoft.com/en-us/library/mt125501.aspx
        auto hr = s_DynamicD3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));

        if (CheckAPIFailed(hr, "D3D12GetDebugInterface"))
            return false;

        debugInterface->EnableDebugLayer();

        // GPU based validation
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
        hr = debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1));

        if (CheckAPIFailed(hr, "ID3D12Debug1::QueryInterface"))
            return false;

        // TODO: once we can use a windows SDK at least 10.0.17134.12, also add support for DRED
        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/use-dred

        debugInterface1->SetEnableGPUBasedValidation(true);

        static PFN_D3D12_CREATE_DEVICE s_DynamicD3D12CreateDevice = nullptr;

        if (!s_DynamicD3D12CreateDevice) {
            s_DynamicD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12CreateDevice")));
            if (!s_DynamicD3D12CreateDevice)
                return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;

        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;

        if (DXCommon::GetDXGIFactory<IDXGIFactory4>(dxgiFactory.GetAddressOf())) {
            if (adapter < 0) {
                // WARP
                hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));

                if (CheckAPIFailed(hr, "IDXGIFactory4::EnumWarpAdapter"))
                    return false;
            } else {
                hr = dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf());

                if (CheckAPIFailed(hr, "IDXGIFactory4::EnumAdapters"))
                    return false;
            }
        } else {
            LOGE << "Failed to to create IDXGIFactory4";
            return false;
        }

        auto minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        hr = s_DynamicD3D12CreateDevice(pAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&_device));

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

    bool DX12::Dispatch(const CommandHandle& cmd)
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

            DX12CommandInitDesc initDesc = {
                _device,
                _commandAllocatorCompute,
                foundShader->second,
                foundRS->second,
                (_type & ERenderer::RENDERER_WARP) != 0
            };

            if (!dxCmd->Initialize(initDesc)) {
                return false;
            }
        } else {
            dxCmd->_cmdList->Reset(_commandAllocatorCompute.Get(), dxCmd->_pipelineState.Get());
        }

        dxCmd->_cmdList->SetPipelineState(dxCmd->_pipelineState.Get());
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

            CD3DX12_RESOURCE_BARRIER pushBarrier = CD3DX12_RESOURCE_BARRIER::Transition(dxUAV->_resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            dxCmd->_cmdList->ResourceBarrier(1, &pushBarrier);

            dxCmd->_cmdList->SetComputeRootUnorderedAccessView(found->second, dxUAV->_resource->GetGPUVirtualAddress());
            dxCmd->_cmdList->Dispatch(cmd->dispatch[0], cmd->dispatch[1], cmd->dispatch[2]);

            CD3DX12_RESOURCE_BARRIER popBarrier = CD3DX12_RESOURCE_BARRIER::Transition(dxUAV->_resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
            dxCmd->_cmdList->ResourceBarrier(1, &popBarrier);
        }

        auto hr = dxCmd->_cmdList->Close();

        if (CheckAPIFailed(hr, "ID3D12GraphicsCommandList::Close"))
            return false;

        ExecuteCommand(dxCmd->_cmdList);

        return true;
    }

    bool DX12::ExecuteCommand(const DX12GraphicsCommandList& cmdList)
    {
        // for now we can to execute the command list right away
        ID3D12CommandList* pCommandLists[] = { cmdList.Get() };

        _commandQueue->ExecuteCommandLists(1, pCommandLists);

        uint64_t fenceValue = InterlockedIncrement(&_fenceValue);
        auto hr = _commandQueue->Signal(_fence.Get(), fenceValue);

        if (CheckAPIFailed(hr, "ID3D12CommandQueue::Signal"))
            return false;

        hr = _fence->SetEventOnCompletion(fenceValue, _fenceEvent);

        if (CheckAPIFailed(hr, "ID3D12Fence::SetEventOnCompletion"))
            return false;

        WaitForSingleObject(_fenceEvent, INFINITE);

        return true;
    }

    void DX12::Finalize()
    {
        WaitForSingleObject(_fenceEvent, 5000);
        CloseHandle(_fenceEvent);

        ObjectTracker::Instance().ReleaseObjects();
    }

    bool DX12::Initialize(const std::vector<std::string>& shaderPaths)
    {
        auto adapter = 0;

        if ((_type & ERenderer::RENDERER_WARP) != 0)
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

        // heap
        D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
        srvUavHeapDesc.NumDescriptors = MAX_DESCRIPTOR_COUNT;
        srvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr;

        hr = _device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&_srvUAVHeap));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap"))
            return false;

        _srvUAVDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Create command allocators
        hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&_commandAllocatorCompute));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator (compute)"))
            return false;

        hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&_commandAllocatorCopy));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator (copy)"))
            return false;

        // command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };

        hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandQueue"))
            return false;

        // fence
        hr = _device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&_fence));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateFence"))
            return false;

        // fence event
        _fenceEvent = CreateEvent(nullptr, false, false, L"Ninniku Fence Event");
        if (_fenceEvent == nullptr) {
            LOGE << "Failed to create fence event";
            return false;
        }

        // copy command list
        // only support a single GPU for now
        hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, _commandAllocatorCopy.Get(), nullptr, IID_PPV_ARGS(&_copyCmdList));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList (copy)"))
            return false;

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
                IDxcBlob* pShaderBlob;
                hr = pContainerReflection->GetPartContent(i, &pShaderBlob);

                if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartContent"))
                    return false;

                auto pHeader = reinterpret_cast<const hlsl::DxilProgramHeader*>(pShaderBlob->GetBufferPointer());

                if (!IsValidDxilProgramHeader(pHeader, static_cast<uint32_t>(pShaderBlob->GetBufferSize()))) {
                    LOGE << "DXIL program header is invalid";
                    return false;
                }

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
                Microsoft::WRL::ComPtr<IDxcBlob> pRSBlob;
                hr = pContainerReflection->GetPartContent(i, &pRSBlob);

                if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartContent"))
                    return false;

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
        auto fmt = boost::format("Found %1% compiled shaders in %2%") % numFiles % shaderPath;

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

    MappedResourceHandle DX12::Map(const BufferHandle& bObj)
    {
        auto impl = static_cast<const DX12BufferImpl*>(bObj.get());
        auto internal = impl->_impl.lock();

        void* data = nullptr;

        auto hr = internal->_buffer->Map(0, nullptr, &data);

        if (CheckAPIFailed(hr, "ID3D12Resource::Map"))
            return std::unique_ptr<MappedResource>();

        return std::make_unique<DX12MappedResource>(internal->_buffer, nullptr, 0, data);
    }

    MappedResourceHandle DX12::Map([[maybe_unused]]const TextureHandle& tObj, [[maybe_unused]]const uint32_t index)
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