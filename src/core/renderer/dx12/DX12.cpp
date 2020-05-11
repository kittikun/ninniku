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
#pragma comment(lib, "dxguid.lib")

#include "../../../globals.h"
#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../../../utils/objectTracker.h"
#include "../DXCommon.h"

#pragma warning(push)
#pragma warning(disable:4100)
#include "pix3.h"
#pragma warning(pop)

#include <atlbase.h>
#include <comdef.h>
#include <dxgi1_4.h>
#include <dxc/dxcapi.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/Support/d3dx12.h>
#include <boost/crc.hpp>

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

        auto dstReadback = (dstImpl->GetDesc()->viewflags & EResourceViews::RV_CPU_READ) != 0;

        // dst is already D3D12_RESOURCE_STATE_COPY_DEST
        auto barrierCount = dstReadback ? 1 : 2;

        std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->_buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
        };

        // we assume that dst is already in D3D12_RESOURCE_STATE_COPY_DEST
        _transitionCmdList->ResourceBarrier(barrierCount, barriers.data());
        _transitionCmdList->Close();
        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        _copyCmdList->CopyResource(dstInternal->_buffer.Get(), srcInternal->_buffer.Get());
        _copyCmdList->Close();
        ExecuteCommand(_copyCommandQueue, _copyCmdList);
        _copyCmdList->Reset(_copyCommandAllocator.Get(), nullptr);

        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_buffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };

        _transitionCmdList->ResourceBarrier(barrierCount, barriers.data());
        _transitionCmdList->Close();
        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresource(const CopyTextureSubresourceParam& params)
    {
        auto srcImpl = static_cast<const DX12TextureImpl*>(params.src);
        auto srcInternal = srcImpl->_impl.lock();
        auto srcDesc = params.src->GetDesc();
        uint32_t srcSub = D3D12CalcSubresource(params.srcMip, params.srcFace, 0, srcDesc->numMips, srcDesc->arraySize);
        auto srcLoc = CD3DX12_TEXTURE_COPY_LOCATION(srcInternal->_texture.Get(), srcSub);

        auto dstImpl = static_cast<const DX12TextureImpl*>(params.dst);
        auto dstInternal = dstImpl->_impl.lock();
        auto dstDesc = params.dst->GetDesc();
        uint32_t dstSub = D3D12CalcSubresource(params.dstMip, params.dstFace, 0, dstDesc->numMips, dstDesc->arraySize);
        auto dstLoc = CD3DX12_TEXTURE_COPY_LOCATION(dstInternal->_texture.Get(), dstSub);

        // transitions states in
        std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_texture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->_texture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON)
        };

        _transitionCmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        _transitionCmdList->Close();

        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        // actual copy
        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->_texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)
        };

        _copyCmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        _copyCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
        _copyCmdList->Close();

        ExecuteCommand(_copyCommandQueue, _copyCmdList);
        _copyCmdList->Reset(_copyCommandAllocator.Get(), nullptr);

        // transitions states out
        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->_texture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };

        _transitionCmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        _transitionCmdList->Close();

        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        return std::make_tuple(srcSub, dstSub);
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresourceToBuffer(const CopyTextureSubresourceToBufferParam& params)
    {
        auto texImpl = static_cast<const DX12TextureImpl*>(params.tex);
        auto texInternal = texImpl->_impl.lock();
        auto texDesc = params.tex->GetDesc();
        uint32_t texSub = D3D12CalcSubresource(params.texMip, params.texFace, 0, texDesc->numMips, texDesc->arraySize);
        auto texLoc = CD3DX12_TEXTURE_COPY_LOCATION(texInternal->_texture.Get(), texSub);

        auto bufferImpl = static_cast<const DX12BufferImpl*>(params.buffer);
        auto bufferInternal = bufferImpl->_impl.lock();

        auto format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(texDesc->format));
        auto width = texDesc->width >> params.texMip;
        auto height = texDesc->height >> params.texMip;
        auto rowPitch = Align(DXGIFormatToNumBytes(format) * width, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        uint32_t offset = 0;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};

        bufferFootprint.Footprint = CD3DX12_SUBRESOURCE_FOOTPRINT{ format, width, height, 1, rowPitch };
        bufferFootprint.Offset = offset;

        auto bufferLoc = CD3DX12_TEXTURE_COPY_LOCATION{ bufferInternal->_buffer.Get(), bufferFootprint };

        std::array<D3D12_RESOURCE_BARRIER, 1> barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(texInternal->_texture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),
        };

        _transitionCmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        _transitionCmdList->Close();
        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        auto common2src = CD3DX12_RESOURCE_BARRIER::Transition(texInternal->_texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

        _copyCmdList->ResourceBarrier(1, &common2src);
        _copyCmdList->CopyTextureRegion(&bufferLoc, 0, 0, 0, &texLoc, nullptr);
        _copyCmdList->Close();
        ExecuteCommand(_copyCommandQueue, _copyCmdList);
        _copyCmdList->Reset(_copyCommandAllocator.Get(), nullptr);

        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(texInternal->_texture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            //CD3DX12_RESOURCE_BARRIER::Transition(bufferInternal->_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };

        _transitionCmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        _transitionCmdList->Close();
        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        return std::make_tuple(rowPitch, offset);
    }

    BufferHandle DX12::CreateBuffer(const BufferParamHandle& params)
    {
        auto isSRV = (params->viewflags & EResourceViews::RV_SRV) != 0;
        auto isUAV = (params->viewflags & EResourceViews::RV_UAV) != 0;
        auto isCPURead = (params->viewflags & EResourceViews::RV_CPU_READ) != 0;
        auto bufferSize = params->numElements * params->elementSize;

        LOGDF(boost::format("Creating Buffer: ElementSize=%1%, NumElements=%2%, Size=%3%") % params->elementSize % params->numElements % bufferSize);

        auto impl = std::make_shared<DX12BufferInternal>();

        _tracker.RegisterObject(impl);

        impl->_desc = params;

        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_RESOURCE_STATES resState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        D3D12_HEAP_TYPE heapFlags = D3D12_HEAP_TYPE_DEFAULT;

        if (isUAV)
            resFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if (isCPURead) {
            resState = D3D12_RESOURCE_STATE_COPY_DEST;
            heapFlags = D3D12_HEAP_TYPE_READBACK;
        }

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(heapFlags);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resFlags);

        auto hr = _device->CreateCommittedResource(
                      &heapProperties,
                      D3D12_HEAP_FLAG_NONE,
                      &desc,
                      resState,
                      nullptr,
                      IID_PPV_ARGS(impl->_buffer.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return BufferHandle();

        if (isSRV) {
            auto srv = new DX12ShaderResourceView(std::numeric_limits<uint32_t>::max());

            srv->_resource = impl;

            impl->_srv.reset(srv);
        }

        if (isUAV) {
            auto uav = new DX12UnorderedAccessView(std::numeric_limits<uint32_t>::max());

            uav->_resource = impl;
            impl->_uav.reset(uav);
        }

        return std::make_unique<DX12BufferImpl>(impl);
    }

    BufferHandle DX12::CreateBuffer(const TextureParamHandle& params)
    {
        // Special case because we cannot read back a texture from the GPU since dx12
        // intended to be used with CopyTextureSubresourceToBuffer
        auto bytesPPx = DXGIFormatToNumBytes(NinnikuTFToDXGIFormat(params->format));
        auto rowPitch = Align(bytesPPx * params->width, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        auto bufferSize = rowPitch * params->height;

        LOGDF(boost::format("Creating Buffer from Texture: Width=%1%, Height=%2%, Size=%3%") % params->width % params->height % bufferSize);

        auto impl = std::make_shared<DX12BufferInternal>();

        _tracker.RegisterObject(impl);

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_NONE);

        auto hr = _device->CreateCommittedResource(
                      &heapProperties,
                      D3D12_HEAP_FLAG_NONE,
                      &desc,
                      D3D12_RESOURCE_STATE_COPY_DEST,
                      nullptr,
                      IID_PPV_ARGS(impl->_buffer.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return BufferHandle();

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

    bool DX12::CreateCommandContexts()
    {
        for (auto& kvp : _resourceBindings) {
            boost::crc_32_type res;

            res.process_bytes(kvp.first.c_str(), kvp.first.size());

            auto context = std::make_shared<DX12CommandInternal>(res.checksum());

            // find the shader bytecode
            auto foundShader = _shaders.find(kvp.first);

            if (foundShader == _shaders.end()) {
                LOGEF(boost::format("CreateCommandContexts: could not find shader \"%1%\"") % kvp.first);
                return false;
            }

            auto foundRS = _rootSignatures.find(kvp.first);

            if (foundRS == _rootSignatures.end()) {
                LOGEF(boost::format("CreateCommandContexts: could not find the root signature for shader \"%1%\"") % kvp.first);
                return false;
            }

            // keep a reference to the root signature for access without lookup
            context->_rootSignature = foundRS->second;

            auto foundBindings = _resourceBindings.find(kvp.first);

            if (foundBindings == _resourceBindings.end()) {
                LOGEF(boost::format("CreateCommandContexts: could not find the resource bindings for shader \"%1%\"") % kvp.first);
                return false;
            }

            // Create pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
            desc.CS = foundShader->second;
            desc.pRootSignature = foundRS->second.Get();

            desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;

            LOGDF(boost::format("Creating pipeling state with byte code: Pointer=%1%, Size=%2%") % desc.CS.pShaderBytecode % desc.CS.BytecodeLength);

            auto hr = _device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&context->_pipelineState));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateComputePipelineState"))
                return false;

            context->_pipelineState->SetName(strToWStr(kvp.first).c_str());

            // Create command allocators
            hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&context->_cmdAllocator));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator (compute)"))
                return false;

            // Create command list
            // only support a single GPU for now
            // we also only support compute for now and don't expect any pipeline state changes for now
            hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, context->_cmdAllocator.Get(), context->_pipelineState.Get(), IID_PPV_ARGS(&context->_cmdList));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList"))
                return false;

            context->_cmdList->SetName(strToWStr(kvp.first).c_str());

            // no need to track contexts because they will be released upon destruction anyway
            _commandContexts.emplace(res.checksum(), std::move(context));
        }

        return true;
    }

    bool DX12::CreateConstantBuffer(DX12ConstantBuffer& cbuffer, const std::string_view& name, void* data, const uint32_t size)
    {
        // Constant buffers are created during the shaders resource bindings parsing but
        // we don't know the size at that point so we must allocate resource at the first update

        if ((cbuffer._size != 0) && (cbuffer._size != size)) {
            LOGEF(boost::format("Constant buffer \"%1%\"'s size have changed. Was %2% now %3%") % name % cbuffer._size % size);
            return false;
        }

        cbuffer._size = Align(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(cbuffer._size);

        auto hr = _device->CreateCommittedResource(
                      &heapProperties,
                      D3D12_HEAP_FLAG_NONE,
                      &desc,
                      D3D12_RESOURCE_STATE_COPY_DEST,
                      nullptr,
                      IID_PPV_ARGS(cbuffer._resource.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource (resource)"))
            return false;

        auto fmt = boost::format("Constant Buffer \"%1%\"") % name;
        cbuffer._resource->SetName(strToWStr(boost::str(fmt)).c_str());

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        hr = _device->CreateCommittedResource(
                 &heapProperties,
                 D3D12_HEAP_FLAG_NONE,
                 &desc,
                 D3D12_RESOURCE_STATE_GENERIC_READ,
                 nullptr,
                 IID_PPV_ARGS(cbuffer._upload.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource (upload)"))
            return false;

        fmt = boost::format("Constant Buffer \"%1%\" Upload") % name;
        cbuffer._upload->SetName(strToWStr(boost::str(fmt)).c_str());

        D3D12_SUBRESOURCE_DATA subdata = {};
        subdata.pData = data;
        subdata.RowPitch = cbuffer._size;
        subdata.SlicePitch = subdata.RowPitch;

        UpdateSubresources(_copyCmdList.Get(), cbuffer._resource.Get(), cbuffer._upload.Get(), 0, 0, 1, &subdata);
        _copyCmdList->Close();
        ExecuteCommand(_copyCommandQueue, _copyCmdList);
        _copyCmdList->Reset(_copyCommandAllocator.Get(), nullptr);

        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(cbuffer._resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        _transitionCmdList->ResourceBarrier(1, &transition);
        _transitionCmdList->Close();
        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        return true;
    }

    DebugMarkerHandle DX12::CreateDebugMarker(const std::string_view& name) const
    {
        return std::make_unique<DX12DebugMarker>(name);
    }

    bool DX12::CreateDevice(int adapter)
    {
        LOGD << "Creating ID3D12Device..";

        auto hModD3D12 = LoadLibrary(L"d3d12.dll");

        if (!hModD3D12)
            return false;

        HRESULT hr;

        if (Globals::Instance()._useDebugLayer) {
            static PFN_D3D12_GET_DEBUG_INTERFACE s_DynamicD3D12GetDebugInterface = nullptr;

            if (!s_DynamicD3D12GetDebugInterface) {
                s_DynamicD3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12GetDebugInterface")));
                if (!s_DynamicD3D12GetDebugInterface)
                    return false;
            }

            Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;

            // if an exception if thrown here, you might need to install the graphics tools
            // https://msdn.microsoft.com/en-us/library/mt125501.aspx
            hr = s_DynamicD3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));

            if (CheckAPIFailed(hr, "D3D12GetDebugInterface"))
                return false;

            debugInterface->EnableDebugLayer();

            // GPU based validation
            Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
            hr = debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1));

            if (CheckAPIFailed(hr, "ID3D12Debug1::QueryInterface"))
                return false;

            debugInterface1->SetEnableGPUBasedValidation(true);

            CComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;

            hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings));

            if (CheckAPIFailed(hr, "D3D12GetDebugInterface"))
                return false;

            // Turn on auto-breadcrumbs and page fault reporting.
            pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        }

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

    bool DX12::CreateSamplers()
    {
        // samplers, point first
        auto sampler = new DX12SamplerState();

        auto& desc = sampler->_desc;
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressV = desc.AddressW = desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MaxAnisotropy = 1;
        desc.MinLOD = -D3D12_FLOAT32_MAX;
        desc.MaxLOD = -D3D12_FLOAT32_MAX;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

        // Create descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 1;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        auto hr = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sampler->_descriptorHeap));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap (SS_Point)"))
            return false;

        sampler->_descriptorHeap->SetName(L"SS_Point");
        _samplers[static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Point)].reset(sampler);

        // linear
        sampler = new DX12SamplerState();

        desc = sampler->_desc;
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressV = desc.AddressW = desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MaxAnisotropy = 1;
        desc.MinLOD = -D3D12_FLOAT32_MAX;
        desc.MaxLOD = -D3D12_FLOAT32_MAX;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

        sampler->_desc = desc;

        hr = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sampler->_descriptorHeap));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap (SS_Linear)"))
            return false;

        sampler->_descriptorHeap->SetName(L"SS_Linear");
        _samplers[static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Linear)].reset(sampler);

        return true;
    }

    TextureHandle DX12::CreateTexture(const TextureParamHandle& params)
    {
        if ((params->viewflags & EResourceViews::RV_CPU_READ) != 0) {
            LOGE << "Textures cannot be created with EResourceViews::RV_CPU_READ";
            return TextureHandle();
        }

        auto isSRV = (params->viewflags & EResourceViews::RV_SRV) != 0;
        auto isUAV = (params->viewflags & EResourceViews::RV_UAV) != 0;
        auto is3d = params->depth > 1;
        auto is1d = params->height == 1;
        auto is2d = (!is3d) && (!is1d);
        auto isCube = is2d && (params->arraySize == CUBEMAP_NUM_FACES);
        auto isCubeArray = is2d && (params->arraySize > CUBEMAP_NUM_FACES) && ((params->arraySize % CUBEMAP_NUM_FACES) == 0);
        auto haveData = !params->imageDatas.empty();

        auto fmt = boost::format("Creating Texture: Size=%1%x%2%, Mips=%3% InitialData=%4%") % params->width % params->height % params->numMips % params->imageDatas.size();
        LOGD << boost::str(fmt);

        auto impl = std::make_shared<DX12TextureInternal>();

        _tracker.RegisterObject(impl);

        impl->_desc = params;

        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_RESOURCE_STATES resState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        D3D12_HEAP_TYPE heapFlags = D3D12_HEAP_TYPE_DEFAULT;

        if (isUAV)
            resFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if (haveData)
            resState = D3D12_RESOURCE_STATE_COMMON;

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(heapFlags);
        CD3DX12_RESOURCE_DESC desc;

        if (is1d) {
            desc = CD3DX12_RESOURCE_DESC::Tex1D(
                       static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format)),
                       params->width,
                       static_cast<uint16_t>(params->arraySize),
                       static_cast<uint16_t>(params->numMips),
                       resFlags
                   );
        } else if (is2d) {
            desc = CD3DX12_RESOURCE_DESC::Tex2D(
                       static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format)),
                       params->width,
                       params->height,
                       static_cast<uint16_t>(params->arraySize),
                       static_cast<uint16_t>(params->numMips),
                       1,
                       0,
                       resFlags
                   );
        } else {
            // is3d
            desc = CD3DX12_RESOURCE_DESC::Tex3D(
                       static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format)),
                       params->width,
                       params->height,
                       static_cast<uint16_t>(params->depth),
                       static_cast<uint16_t>(params->numMips),
                       resFlags
                   );
        }

        auto hr = _device->CreateCommittedResource(
                      &heapProperties,
                      D3D12_HEAP_FLAG_NONE,
                      &desc,
                      resState,
                      nullptr,
                      IID_PPV_ARGS(impl->_texture.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return TextureHandle();

        if (haveData) {
            DX12Resource upload;

            auto numImageImpls = static_cast<uint32_t>(params->imageDatas.size());
            auto reqSize = GetRequiredIntermediateSize(impl->_texture.Get(), 0, numImageImpls);

            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            desc = CD3DX12_RESOURCE_DESC::Buffer(reqSize);

            hr = _device->CreateCommittedResource(
                     &heapProperties,
                     D3D12_HEAP_FLAG_NONE,
                     &desc,
                     D3D12_RESOURCE_STATE_GENERIC_READ,
                     nullptr,
                     IID_PPV_ARGS(upload.GetAddressOf()));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource (texture upload buffer)"))
                return TextureHandle();

            upload->SetName(L"Texture Upload Buffer");

            std::vector<D3D12_SUBRESOURCE_DATA> initialData(numImageImpls);

            for (uint32_t i = 0; i < numImageImpls; ++i) {
                auto& subParam = params->imageDatas[i];
                auto& initData = initialData[i];

                initData.pData = subParam.data;
                initData.RowPitch = subParam.rowPitch;
                initData.SlicePitch = subParam.depthPitch;
            }

            auto push = CD3DX12_RESOURCE_BARRIER::Transition(impl->_texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

            _copyCmdList->ResourceBarrier(1, &push);

            UpdateSubresources(_copyCmdList.Get(), impl->_texture.Get(), upload.Get(), 0, 0, numImageImpls, initialData.data());

            auto pop = CD3DX12_RESOURCE_BARRIER::Transition(impl->_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            _transitionCmdList->ResourceBarrier(1, &pop);
            _copyCmdList->Close();
            _transitionCmdList->Close();

            ExecuteCommand(_copyCommandQueue, _copyCmdList);
            ExecuteCommand(_transitionCommandQueue, _transitionCmdList);

            _copyCmdList->Reset(_copyCommandAllocator.Get(), nullptr);
            _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);
        }

        if (isSRV) {
            auto lmbd = [&](const std::shared_ptr<DX12TextureInternal>& internal, SRVHandle & target, uint32_t arrayIndex) {
                auto srv = new DX12ShaderResourceView(arrayIndex);

                srv->_resource = internal;
                target.reset(srv);
            };

            if (isCubeArray) {
                lmbd(impl, impl->_srvCubeArray, std::numeric_limits<uint32_t>::max());
            }

            if (isCube) {
                // To sample texture as cubemap
                lmbd(impl, impl->_srvCube, std::numeric_limits<uint32_t>::max());

                // To sample texture as array, one for each miplevel
                impl->_srvArray.resize(params->numMips);

                for (uint32_t i = 0; i < params->numMips; ++i) {
                    lmbd(impl, impl->_srvArray[i], i);
                }

                lmbd(impl, impl->_srvArrayWithMips, std::numeric_limits<uint32_t>::max());
            } else if (params->arraySize > 1) {
                // one for each miplevel
                impl->_srvArray.resize(params->numMips);

                for (uint32_t i = 0; i < params->numMips; ++i) {
                    lmbd(impl, impl->_srvArray[i], i);
                }
            } else {
                lmbd(impl, impl->_srvDefault, std::numeric_limits<uint32_t>::max());
            }
        }

        if (isUAV) {
            impl->_uav.resize(params->numMips);

            // we have to create an UAV for each miplevel
            for (uint32_t i = 0; i < params->numMips; ++i) {
                auto uav = new DX12UnorderedAccessView(i);

                uav->_resource = impl;
                impl->_uav[i].reset(uav);
            }
        }

        return std::make_unique<DX12TextureImpl>(impl);
    }

    bool DX12::Dispatch(const CommandHandle& cmd)
    {
        DX12Command* dxCmd = static_cast<DX12Command*>(cmd.get());

        auto shaderHash = dxCmd->GetHashShader();

        // if the user changed the corresponding shader unbind iti
        if (!dxCmd->_impl.expired() && (dxCmd->_impl.lock()->_contextShaderHash != shaderHash)) {
            dxCmd->_impl.reset();
        }

        if (dxCmd->_impl.expired()) {
            // find the appropriate context
            auto foundContext = _commandContexts.find(shaderHash);

            if (foundContext == _commandContexts.end()) {
                auto fmt = boost::format("Dispatch error: could not find command context for shader \"%1%\"") % cmd->shader;
                LOGE << boost::str(fmt);
                return false;
            }

            dxCmd->_impl = foundContext->second;
        }

        auto context = dxCmd->_impl.lock();

        // resource bindings for this shader
        auto foundBindings = _resourceBindings.find(cmd->shader);

        if (foundBindings == _resourceBindings.end()) {
            LOGEF(boost::format("Dispatch error: could not find resource binding table for shader \"%1%\"") % cmd->shader);
            return false;
        }

        auto& bindings = foundBindings->second;
        auto hash = dxCmd->GetHashBindings();

        // look for the subcontext
        auto foundHash = context->_subContexts.find(hash);

        DX12CommandSubContext* subContext = nullptr;

        if (foundHash == context->_subContexts.end()) {
            // create and initialize subcontext
            if (!context->CreateSubContext(_device, hash, cmd->shader, static_cast<uint32_t>(bindings.size())))
                return false;

            subContext = &context->_subContexts[hash];
            subContext->Initialize(_device, dxCmd, bindings, _cBuffers);
        } else {
            subContext = &foundHash->second;
        }

        context->_cmdList->SetPipelineState(context->_pipelineState.Get());
        context->_cmdList->SetComputeRootSignature(context->_rootSignature.Get());

        // At most we have 2 extra samplers
        std::array<ID3D12DescriptorHeap*, 3> descriptorHeaps;

        // order is fixed, context then samplers
        auto descriptorHeapCount = 1u;
        descriptorHeaps[0] = subContext->_descriptorHeap.Get();

        if (!cmd->ssBindings.empty()) {
            for (auto& ss : cmd->ssBindings) {
                auto dxSS = static_cast<const DX12SamplerState*>(ss.second);
                auto found = bindings.find(ss.first);

                if (found == bindings.end()) {
                    LOGEF(boost::format("DX12CommandInternal::Initialize: could not find SS binding \"%1%\"") % ss.first);
                    return false;
                }

                auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE{ dxSS->_descriptorHeap->GetCPUDescriptorHandleForHeapStart() };

                _device->CreateSampler(&dxSS->_desc, handle);

                descriptorHeaps[descriptorHeapCount++] = dxSS->_descriptorHeap.Get();
            }
        }

        context->_cmdList->SetDescriptorHeaps(descriptorHeapCount, descriptorHeaps.data());

        for (auto i = 0u; i < descriptorHeapCount; ++i) {
            context->_cmdList->SetComputeRootDescriptorTable(i, descriptorHeaps[i]->GetGPUDescriptorHandleForHeapStart());
        }

        // resources view are bound in the descriptor heap but we still need to transition their states
        for (auto& kvp : cmd->uavBindings) {
            auto dxUAV = static_cast<const DX12UnorderedAccessView*>(kvp.second);
            auto found = bindings.find(kvp.first);

            if (found == bindings.end()) {
                auto fmt = boost::format("Dispatch error: could not find resource bindings for \"%1%\" in \"%2%\"") % kvp.first % cmd->shader;
                LOGE << boost::str(fmt);
                return false;
            }

            if (std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->_resource)) {
                auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->_resource);
                auto locked = weak.lock();

                std::array<D3D12_RESOURCE_BARRIER, 1> barriers = {
                    //CD3DX12_RESOURCE_BARRIER::UAV(locked->_buffer.Get()),
                    CD3DX12_RESOURCE_BARRIER::Transition(locked->_buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
                };

                context->_cmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            } else {
                // DX12TextureInternal
                auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->_resource);
                auto locked = weak.lock();

                std::array<D3D12_RESOURCE_BARRIER, 1> barriers = {
                    //CD3DX12_RESOURCE_BARRIER::UAV(locked->_texture.Get()),
                    CD3DX12_RESOURCE_BARRIER::Transition(locked->_texture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
                };

                context->_cmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }
        }

        context->_cmdList->Dispatch(cmd->dispatch[0], cmd->dispatch[1], cmd->dispatch[2]);

        // revert transition back
        for (auto& kvp : cmd->uavBindings) {
            auto dxUAV = static_cast<const DX12UnorderedAccessView*>(kvp.second);

            // bindings correctness was already checked during push so skip them

            if (std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->_resource)) {
                auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->_resource);
                auto locked = weak.lock();

                std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
                    CD3DX12_RESOURCE_BARRIER::UAV(locked->_buffer.Get()),
                    CD3DX12_RESOURCE_BARRIER::Transition(locked->_buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
                };

                context->_cmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            } else {
                // DX12TextureInternal
                auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->_resource);
                auto locked = weak.lock();

                std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
                    CD3DX12_RESOURCE_BARRIER::UAV(locked->_texture.Get()),
                    CD3DX12_RESOURCE_BARRIER::Transition(locked->_texture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
                };

                context->_cmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }
        }

        auto hr = context->_cmdList->Close();

        if (CheckAPIFailed(hr, "ID3D12GraphicsCommandList::Close"))
            return false;

        ExecuteCommand(_commandQueue, context->_cmdList);

        context->_cmdList->Reset(context->_cmdAllocator.Get(), context->_pipelineState.Get());

        return true;
    }

    bool DX12::ExecuteCommand(const DX12CommandQueue& queue, const DX12GraphicsCommandList& cmdList)
    {
        // for now we can to execute the command list right away
        std::array<ID3D12CommandList*, 1> pCommandLists = { cmdList.Get() };

        queue->ExecuteCommandLists(1, &pCommandLists.front());

        uint64_t fenceValue = InterlockedIncrement(&_fenceValue);
        auto hr = queue->Signal(_fence.Get(), fenceValue);

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

        _tracker.ReleaseObjects();
    }

    bool DX12::Initialize(const std::vector<std::string_view>& shaderPaths)
    {
        auto adapter = 0;

        if ((_type & ERenderer::RENDERER_WARP) != 0)
            adapter = -1;

        if (!CreateDevice(adapter)) {
            LOGE << "Failed to create DX12 device";
            return false;
        }

        // common descriptor heaps
        D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
        srvUavHeapDesc.NumDescriptors = MAX_DESCRIPTOR_COUNT;
        srvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr;

        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = 1;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        hr = _device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&_samplerHeap));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap"))
            return false;

        // Create command allocators
        hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&_commandAllocatorCompute));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator (compute)"))
            return false;

        hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&_copyCommandAllocator));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator (copy)"))
            return false;

        hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_transitionCommandAllocator));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator (transition)"))
            return false;

        // command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };

        hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandQueue"))
            return false;

        queueDesc = { D3D12_COMMAND_LIST_TYPE_COPY, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };

        hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_copyCommandQueue));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandQueue (copy)"))
            return false;

        queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };

        hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_transitionCommandQueue));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandQueue (transition)"))
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

        // command lists
        // only support a single GPU for now

        // copy
        hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, _copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_copyCmdList));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList (copy)"))
            return false;

        _copyCmdList->SetName(L"Copy Command List");

        // resource transition
        hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _transitionCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_transitionCmdList));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList (transition)"))
            return false;

        _copyCmdList->SetName(L"Transition Command List");

        // parse shaders
        if ((shaderPaths.size() > 0)) {
            for (auto& path : shaderPaths) {
                if (!LoadShaders(path)) {
                    auto fmt = boost::format("Failed to load shaders in: %1%") % path;
                    LOGE << boost::str(fmt);
                    return false;
                }
            }
        }

        if (!CreateSamplers())
            return false;

        return true;
    }

    bool DX12::LoadShader([[maybe_unused]]const std::string_view& name, [[maybe_unused]]const void* pData, [[maybe_unused]]const size_t size)
    {
        throw std::exception("not implemented");
    }

    bool DX12::LoadShader(const std::string_view& name, IDxcBlobEncoding* pBlob)
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
        _shaders.emplace(name, CD3DX12_SHADER_BYTECODE(pBlob->GetBufferPointer(), pBlob->GetBufferSize()));

        // Create command contexts for all the shaders we just found
        if (!CreateCommandContexts())
            return false;

        return true;
    }

    /// <summary>
    /// Load all shaders in /data
    /// </summary>
    bool DX12::LoadShaders(const std::string_view& shaderPath)
    {
        // check if directory is valid
        if (!std::filesystem::is_directory(shaderPath)) {
            auto fmt = boost::format("Failed to open directory: %1%") % shaderPath;
            LOGE << boost::str(fmt);

            return false;
        }

        std::string_view ext{ ".dxco" };

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

                if (!LoadShader(name, pBlob.Get())) {
                    LOG_INDENT_END;
                    return false;
                }

                LOG_INDENT_END;
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

    bool DX12::ParseRootSignature(const std::string_view& name, IDxcBlobEncoding* pBlob)
    {
        DX12RootSignature rootSignature;

        auto hr = _device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateRootSignature"))
            return false;

        LOGD << "Found a root signature";

        rootSignature->SetName(strToWStr(name).c_str());

        // for now create a root signature per shader
        _rootSignatures.emplace(name, std::move(rootSignature));

        return true;
    }

    bool DX12::ParseShaderResources(const std::string_view& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection)
    {
        // parse parameter bind slots
        auto fmt = boost::format("Found %1% resources") % numBoundResources;

        LOGD_INDENT_START << boost::str(fmt);

        StringMap<D3D12_SHADER_INPUT_BIND_DESC> bindings;

        for (uint32_t i = 0; i < numBoundResources; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;

            auto hr = pReflection->GetResourceBindingDesc(i, &bindDesc);

            if (CheckAPIFailed(hr, "ID3D12ShaderReflection::GetResourceBindingDesc")) {
                LOGD_INDENT_END;
                return false;
            }

            std::string_view restypeStr;

            switch (bindDesc.Type) {
                case D3D_SIT_CBUFFER:
                    restypeStr = "D3D_SIT_CBUFFER";
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
                    LOGD_INDENT_END;
                    return false;
            }

            fmt = boost::format("Resource: Name=\"%1%\", Type=%2%, Slot=%3%") % bindDesc.Name % restypeStr % bindDesc.BindPoint;

            LOGD << boost::str(fmt);

            // if the texture is a constant buffer, we want to create it a slot for it in the map
            if (bindDesc.Type == D3D_SIT_CBUFFER) {
                _cBuffers.emplace(bindDesc.Name, DX12ConstantBuffer{});
            }

            bindings.emplace(bindDesc.Name, bindDesc);
        }

        _resourceBindings.emplace(name, std::move(bindings));

        LOGD_INDENT_END;

        return true;
    }

    bool DX12::UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size)
    {
        auto found = _cBuffers.find(name);

        if (found == _cBuffers.end()) {
            auto fmt = boost::format("Constant buffer \"%1%\" was not found in any of the shaders parsed") % name;
            LOGE << boost::str(fmt);

            return false;
        }

        if (found->second._resource == nullptr) {
            if (!CreateConstantBuffer(found->second, name, data, size))
                return false;
        } else {
            // constant buffer already exists so just update it

            D3D12_SUBRESOURCE_DATA subdata = {};
            subdata.pData = data;
            subdata.RowPitch = size;
            subdata.SlicePitch = subdata.RowPitch;

            auto transition = CD3DX12_RESOURCE_BARRIER::Transition(found->second._resource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

            _transitionCmdList->ResourceBarrier(1, &transition);
            _transitionCmdList->Close();
            ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
            _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

            UpdateSubresources(_copyCmdList.Get(), found->second._resource.Get(), found->second._upload.Get(), 0, 0, 1, &subdata);
            _copyCmdList->Close();
            ExecuteCommand(_copyCommandQueue, _copyCmdList);
            _copyCmdList->Reset(_copyCommandAllocator.Get(), nullptr);

            transition = CD3DX12_RESOURCE_BARRIER::Transition(found->second._resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            _transitionCmdList->ResourceBarrier(1, &transition);
            _transitionCmdList->Close();
            ExecuteCommand(_transitionCommandQueue, _transitionCmdList);
            _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);
        }

        _transitionCmdList->Close();

        // look later at bundling those together
        ExecuteCommand(_transitionCommandQueue, _transitionCmdList);

        _transitionCmdList->Reset(_transitionCommandAllocator.Get(), nullptr);

        return true;
    }
} // namespace ninniku