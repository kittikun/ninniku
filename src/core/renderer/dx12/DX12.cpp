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

#include "DX12.h"

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#include "../../../globals.h"
#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../../../utils/object_tracker.h"
#include "dxc_utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "pix3.h"
#pragma clang diagnostic pop

#include <atlbase.h>
#include <bitset>
#include <comdef.h>
#include <dxgi1_4.h>
#include <dxcapi.h>

#include <d3dx12/d3dx12.h>

#include <boost/crc.hpp>

namespace ninniku
{
    DX12::DX12(ERenderer type)
        : type_{ type }
        , fenceValue_{ 0 }
    {
        commands_.reserve(DEFAULT_COMMAND_QUEUE_SIZE);
    }

    bool DX12::CheckFeatureSupport(EDeviceFeature feature, bool& result)
    {
        TRACE_SCOPED_DX12;

        // cache queries
        static std::bitset<DF_COUNT> queried;
        static std::bitset<DF_COUNT> queryRes;

        // use cache
        if (queried[feature]) {
            result = queryRes[feature];
            return true;
        }

        switch (feature) {
            case ninniku::DF_ALLOW_TEARING:
            {
                auto dxgiFactory = DXGI::GetDXGIFactory5();

                if (dxgiFactory != nullptr) {
                    BOOL allowTearing = false;

                    auto hr = dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

                    if (CheckAPIFailed(hr, "CheckFeatureSupport"))
                        return false;

                    result = allowTearing;
                } else {
                    LOGE << "Failed to create IDXGIFactory5";
                    return false;
                }
            }
            break;

            case ninniku::DF_SM6_WAVE_INTRINSICS:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS1 waveIntrinsicsSupport = {};

                auto hr = device_->CheckFeatureSupport((D3D12_FEATURE)D3D12_FEATURE_D3D12_OPTIONS1, &waveIntrinsicsSupport, sizeof(waveIntrinsicsSupport));

                if (CheckAPIFailed(hr, "CheckFeatureSupport"))
                    return false;

                result = waveIntrinsicsSupport.WaveOps;
            }
            break;

            default:
                LOGE << "Unsupported EDeviceFeature";
                return false;
        }

        queried[feature] = true;
        queryRes[feature] = result;

        return true;
    }

    bool DX12::ClearRenderTarget(const ClearRenderTargetParam& params)
    {
        auto cmdList = CreateCommandList(QT_DIRECT);

        auto rt = static_cast<const DX12RenderTargetView*>(params.dstRT);

        // transition
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt->texture_.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->gfxCmdList_->ResourceBarrier(1, &barrier);

        // clear
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart(), params.index, rtvDescriptorSize_);
        cmdList->gfxCmdList_->ClearRenderTargetView(rtvHandle, params.color, 0, nullptr);

        // revert transition
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt->texture_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        cmdList->gfxCmdList_->ResourceBarrier(1, &barrier);

        ExecuteCommand(cmdList);

        return true;
    }

    bool DX12::CopyBufferResource(const CopyBufferSubresourceParam& params)
    {
        TRACE_SCOPED_DX12;

        auto srcImpl = static_cast<const DX12BufferImpl*>(params.src);

        if (CheckWeakExpired(srcImpl->impl_))
            return false;

        auto srcInternal = srcImpl->impl_.lock();

        auto dstImpl = static_cast<const DX12BufferImpl*>(params.dst);

        if (CheckWeakExpired(dstImpl->impl_))
            return false;

        auto dstInternal = dstImpl->impl_.lock();

        auto dstReadback = (dstImpl->GetDesc()->viewflags & EResourceViews::RV_CPU_READ) != 0;

        // transition
        auto cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return false;
        }

        // dst is already D3D12_RESOURCE_STATE_COPY_DEST
        auto barrierCount = dstReadback ? 1 : 2;

        std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->buffer_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->buffer_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
        };

        // we assume that dst is already in D3D12_RESOURCE_STATE_COPY_DEST
        cmdList->gfxCmdList_->ResourceBarrier(barrierCount, barriers.data());

        ExecuteCommand(cmdList);

        // copy
        cmdList = CreateCommandList(QT_COPY);

        if (cmdList == nullptr) {
            return false;
        }

        cmdList->gfxCmdList_->CopyResource(dstInternal->buffer_.Get(), srcInternal->buffer_.Get());

        ExecuteCommand(cmdList);

        // transition
        cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return false;
        }

        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->buffer_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->buffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };

        cmdList->gfxCmdList_->ResourceBarrier(barrierCount, barriers.data());

        ExecuteCommand(cmdList);

        return true;
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresource(const CopyTextureSubresourceParam& params)
    {
        TRACE_SCOPED_DX12;

        auto srcImpl = static_cast<const DX12TextureImpl*>(params.src);

        if (CheckWeakExpired(srcImpl->impl_))
            return std::tuple<uint32_t, uint32_t>();

        auto srcInternal = srcImpl->impl_.lock();
        auto srcDesc = params.src->GetDesc();
        uint32_t srcSub = D3D12CalcSubresource(params.srcMip, params.srcFace, 0, srcDesc->numMips, srcDesc->arraySize);
        auto srcLoc = CD3DX12_TEXTURE_COPY_LOCATION(srcInternal->texture_.Get(), srcSub);

        auto dstImpl = static_cast<const DX12TextureImpl*>(params.dst);

        if (CheckWeakExpired(dstImpl->impl_))
            return std::tuple<uint32_t, uint32_t>();

        auto dstInternal = dstImpl->impl_.lock();
        auto dstDesc = params.dst->GetDesc();
        uint32_t dstSub = D3D12CalcSubresource(params.dstMip, params.dstFace, 0, dstDesc->numMips, dstDesc->arraySize);
        auto dstLoc = CD3DX12_TEXTURE_COPY_LOCATION(dstInternal->texture_.Get(), dstSub);

        // transitions states in
        auto cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return std::tuple<uint32_t, uint32_t>();
        }

        std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->texture_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->texture_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON)
        };

        cmdList->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

        ExecuteCommand(cmdList);

        // actual copy
        cmdList = CreateCommandList(QT_COPY);

        if (cmdList == nullptr) {
            return std::tuple<uint32_t, uint32_t>();
        }

        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->texture_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->texture_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)
        };

        cmdList->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        cmdList->gfxCmdList_->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        ExecuteCommand(cmdList);

        // transitions states out
        cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return std::tuple<uint32_t, uint32_t>();
        }

        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(srcInternal->texture_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(dstInternal->texture_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };

        cmdList->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

        ExecuteCommand(cmdList);

        return { srcSub, dstSub };
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresourceToBuffer(const CopyTextureSubresourceToBufferParam& params)
    {
        TRACE_SCOPED_DX12;

        auto texImpl = static_cast<const DX12TextureImpl*>(params.tex);

        if (CheckWeakExpired(texImpl->impl_))
            return std::tuple<uint32_t, uint32_t>();

        auto texInternal = texImpl->impl_.lock();
        auto texDesc = params.tex->GetDesc();
        uint32_t texSub = D3D12CalcSubresource(params.texMip, params.texFace, 0, texDesc->numMips, texDesc->arraySize);
        auto texLoc = CD3DX12_TEXTURE_COPY_LOCATION(texInternal->texture_.Get(), texSub);

        auto bufferImpl = static_cast<const DX12BufferImpl*>(params.buffer);

        if (CheckWeakExpired(bufferImpl->impl_))
            return std::tuple<uint32_t, uint32_t>();

        auto bufferInternal = bufferImpl->impl_.lock();

        auto format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(texDesc->format));
        auto width = texDesc->width >> params.texMip;
        auto height = texDesc->height >> params.texMip;
        auto rowPitch = Align(DXGIFormatToNumBytes(format) * width, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        uint32_t offset = 0;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};

        bufferFootprint.Footprint = CD3DX12_SUBRESOURCE_FOOTPRINT{ format, width, height, 1, rowPitch };
        bufferFootprint.Offset = offset;

        auto bufferLoc = CD3DX12_TEXTURE_COPY_LOCATION{ bufferInternal->buffer_.Get(), bufferFootprint };

        // transition
        auto cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return std::tuple<uint32_t, uint32_t>();
        }

        std::array<D3D12_RESOURCE_BARRIER, 1> barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(texInternal->texture_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),
        };

        cmdList->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        ExecuteCommand(cmdList);

        // copy
        cmdList = CreateCommandList(QT_COPY);

        if (cmdList == nullptr) {
            return std::tuple<uint32_t, uint32_t>();
        }

        auto common2src = CD3DX12_RESOURCE_BARRIER::Transition(texInternal->texture_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

        cmdList->gfxCmdList_->ResourceBarrier(1, &common2src);
        cmdList->gfxCmdList_->CopyTextureRegion(&bufferLoc, 0, 0, 0, &texLoc, nullptr);
        ExecuteCommand(cmdList);

        // transitions
        cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return std::tuple<uint32_t, uint32_t>();
        }

        barriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(texInternal->texture_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        };

        cmdList->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        ExecuteCommand(cmdList);

        return { rowPitch, offset };
    }

    BufferHandle DX12::CreateBuffer(const BufferParamHandle& params)
    {
        TRACE_SCOPED_NAMED_DX12("ninniku::DX12::CreateBuffer (BufferParamHandle)");

        auto isSRV = (params->viewflags & static_cast<uint8_t>(EResourceViews::RV_SRV)) != 0;
        auto isUAV = (params->viewflags & static_cast<uint8_t>(EResourceViews::RV_UAV)) != 0;
        auto isCPURead = (params->viewflags & static_cast<uint8_t>(EResourceViews::RV_CPU_READ)) != 0;
        auto bufferSize = params->numElements * params->elementSize;

        LOGDF(boost::format("Creating Buffer: ElementSize=%1%, NumElements=%2%, Size=%3%") % params->elementSize % params->numElements % bufferSize);

        auto impl = std::make_shared<DX12BufferInternal>();

        tracker_.RegisterObject(impl);

        impl->desc_ = params;

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

        auto hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            resState,
            nullptr,
            IID_PPV_ARGS(impl->buffer_.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return BufferHandle();

        if (isSRV) {
            auto srv = new DX12ShaderResourceView(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

            srv->resource_ = impl;

            impl->srv_.reset(srv);
        }

        if (isUAV) {
            auto uav = new DX12UnorderedAccessView(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

            uav->resource_ = impl;
            impl->uav_.reset(uav);
        }

        return std::make_unique<DX12BufferImpl>(impl);
    }

    BufferHandle DX12::CreateBuffer(const TextureParamHandle& params)
    {
        TRACE_SCOPED_NAMED_DX12("ninniku::DX12::CreateBuffer (TextureParamHandle)");

        // Special case because we cannot read back a texture from the GPU since dx12
        // intended to be used with CopyTextureSubresourceToBuffer
        auto bytesPPx = DXGIFormatToNumBytes(NinnikuTFToDXGIFormat(params->format));
        auto rowPitch = Align(bytesPPx * params->width, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        auto bufferSize = rowPitch * params->height;

        LOGDF(boost::format("Creating Buffer from Texture: Width=%1%, Height=%2%, Size=%3%") % params->width % params->height % bufferSize);

        auto impl = std::make_shared<DX12BufferInternal>();

        tracker_.RegisterObject(impl);

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_NONE);

        auto hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(impl->buffer_.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return BufferHandle();

        return std::make_unique<DX12BufferImpl>(impl);
    }

    BufferHandle DX12::CreateBuffer(const BufferHandle& src)
    {
        TRACE_SCOPED_NAMED_DX12("ninniku::DX12::CreateBuffer (BufferHandle)");

        auto implSrc = static_cast<const DX12BufferImpl*>(src.get());

        if (CheckWeakExpired(implSrc->impl_))
            return BufferHandle();

        auto internalSrc = implSrc->impl_.lock();

        assert(internalSrc->desc_->elementSize % 4 == 0);

        auto marker = CreateDebugMarker("CreateBufferFromBufferObject");

        auto dst = CreateBuffer(internalSrc->desc_);
        auto implDst = static_cast<const DX12BufferImpl*>(dst.get());

        if (CheckWeakExpired(implDst->impl_))
            return BufferHandle();

        auto internalDst = implDst->impl_.lock();

        // copy src to dst
        {
            CopyBufferSubresourceParam copyParams = {};

            copyParams.src = src.get();
            copyParams.dst = dst.get();

            if (!CopyBufferResource(copyParams))
                return BufferHandle();
        }

        // create a temporary object readable from CPU to fill internalDst->_data with a map
        auto stride = internalSrc->desc_->elementSize / 4;
        auto params = internalSrc->desc_->Duplicate();

        // allocate memory
        internalDst->data_.resize(stride * internalSrc->desc_->numElements);
        params->viewflags = RV_CPU_READ;

        auto temp = CreateBuffer(params);

        // copy src to temp
        {
            CopyBufferSubresourceParam copyParams = {};

            copyParams.src = src.get();
            copyParams.dst = temp.get();

            if (!CopyBufferResource(copyParams))
                return BufferHandle();
        }

        auto mapped = Map(temp);
        uint32_t dstPitch = static_cast<uint32_t>(internalDst->data_.size() * sizeof(uint32_t));

        memcpy_s(&internalDst->data_.front(), dstPitch, mapped->GetData(), dstPitch);

        return dst;
    }

    CommandList* DX12::CreateCommandList(EQueueType type)
    {
        TRACE_SCOPED_DX12;

        auto cmd = poolCmd_.malloc();

        new(cmd)CommandList{};

        cmd->type_ = type;

        if (Globals::Instance().safeAndSlowDX12) {
            cmd->gfxCmdList_ = queues_[type].cmdList;
        } else {
            HRESULT hr = E_FAIL;

            hr = device_->CreateCommandList(0, QueueTypeToDX12ComandListType(type), queues_[type].cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmd->gfxCmdList_));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList"))
                return nullptr;
        }

        return cmd;
    }

    bool DX12::CreateCommandContexts()
    {
        TRACE_SCOPED_DX12;

        for (auto& kvp : resourceBindings_) {
            boost::crc_32_type res;

            res.process_bytes(kvp.first.c_str(), kvp.first.size());

            auto context = std::make_shared<DX12CommandInternal>(res.checksum());

            // find the shader bytecode
            auto foundShader = shaders_.find(kvp.first);

            if (foundShader == shaders_.end()) {
                LOGEF(boost::format("CreateCommandContexts: could not find shader \"%1%\"") % kvp.first);
                return false;
            }

            auto foundRS = rootSignatures_.find(kvp.first);

            if (foundRS == rootSignatures_.end()) {
                LOGEF(boost::format("CreateCommandContexts: could not find the root signature for shader \"%1%\"") % kvp.first);
                return false;
            }

            // keep a reference to the root signature for access without lookup
            context->rootSignature_ = foundRS->second;

            auto foundBindings = resourceBindings_.find(kvp.first);

            if (foundBindings == resourceBindings_.end()) {
                LOGEF(boost::format("CreateCommandContexts: could not find the resource bindings for shader \"%1%\"") % kvp.first);
                return false;
            }

            // Create pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
            desc.CS = foundShader->second;
            desc.pRootSignature = foundRS->second.Get();

            if (type_ == ERenderer::RENDERER_WARP_DX12)
                desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;

            LOGDF(boost::format("Creating pipeling state with byte code: Pointer=%1%, Size=%2%") % desc.CS.pShaderBytecode % desc.CS.BytecodeLength);

            auto hr = device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&context->pipelineState_));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateComputePipelineState"))
                return false;

            context->pipelineState_->SetName(strToWStr(kvp.first).c_str());

            // no need to track contexts because they will be released upon destruction anyway
            commandContexts_.emplace(res.checksum(), std::move(context));
        }

        return true;
    }

    bool DX12::CreateConstantBuffer(DX12ConstantBuffer& cbuffer, const uint32_t size)
    {
        TRACE_SCOPED_DX12;

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

        auto hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_PPV_ARGS(cbuffer.resource_.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource (resource)"))
            return false;

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(cbuffer.upload_.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource (upload)"))
            return false;

        return true;
    }

    DebugMarkerHandle DX12::CreateDebugMarker(const std::string_view& name) const
    {
        return std::make_unique<DX12DebugMarker>(name);
    }

    bool DX12::CreateDevice(int adapter)
    {
        TRACE_SCOPED_DX12;

        LOGD << "Creating ID3D12Device..";

        auto hModD3D12 = LoadLibrary(L"d3d12.dll");

        if (!hModD3D12)
            return false;

        HRESULT hr;

        if (Globals::Instance().useDebugLayer_) {
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

            if (CheckAPIFailed(hr, "ID3D12Debug1::QueryInterface (GPU validation)"))
                return false;

            debugInterface1->SetEnableGPUBasedValidation(true);

            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;

            hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings));

            if (FAILED(hr)) {
                LOGW << "Couldn't initialize Device Removed Extended Data (DRED), you need to update your Windows 10 version to at least 1903 to use it";
            } else {
                // Turn on auto-breadcrumbs and page fault reporting.
                pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }
        }

        static PFN_D3D12_CREATE_DEVICE s_DynamicD3D12CreateDevice = nullptr;

        if (!s_DynamicD3D12CreateDevice) {
            s_DynamicD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D12, "D3D12CreateDevice")));
            if (!s_DynamicD3D12CreateDevice)
                return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
        auto dxgiFactory = DXGI::GetDXGIFactory5();

        if (dxgiFactory != nullptr) {
            if (adapter < 0) {
                // WARP
                hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));

                if (CheckAPIFailed(hr, "IDXGIFactory5::EnumWarpAdapter"))
                    return false;
            } else {
                hr = dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf());

                if (CheckAPIFailed(hr, "IDXGIFactory5::EnumAdapters"))
                    return false;
            }
        } else {
            LOGE << "Failed to create IDXGIFactory5";
            return false;
        }

        auto minFeatureLevel = D3D_FEATURE_LEVEL_12_1;

        hr = s_DynamicD3D12CreateDevice(pAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&device_));

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
        TRACE_SCOPED_DX12;

        // samplers, point first
        auto sampler = new DX12SamplerState();

        auto& desc = sampler->desc_;
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

        auto hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sampler->descriptorHeap_));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap (SS_Point)"))
            return false;

        sampler->descriptorHeap_->SetName(L"SS_Point");
        samplers_[static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Point)].reset(sampler);

        // linear
        sampler = new DX12SamplerState();

        desc = sampler->desc_;
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressV = desc.AddressW = desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MaxAnisotropy = 1;
        desc.MinLOD = -D3D12_FLOAT32_MAX;
        desc.MaxLOD = -D3D12_FLOAT32_MAX;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

        sampler->desc_ = desc;

        hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sampler->descriptorHeap_));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap (SS_Linear)"))
            return false;

        sampler->descriptorHeap_->SetName(L"SS_Linear");
        samplers_[static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Linear)].reset(sampler);

        return true;
    }

    SwapChainHandle DX12::CreateSwapChain(const SwapchainParam& params)
    {
        auto impl = std::make_shared<DX12SwapChainInternal>();

        tracker_.RegisterObject(impl);

        bool allowTearing;

        if (!CheckFeatureSupport(DF_ALLOW_TEARING, allowTearing))
            return SwapChainHandle();

        if (!DXGI::CreateSwapchain(queues_[QT_DIRECT].cmdQueue.Get(), params, allowTearing, impl->swapchain_))
            return SwapChainHandle();

        // Create descriptor heaps.
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = params.bufferCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            auto hr = device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));

            if (CheckAPIFailed(hr, "Failed to create swap chain's RTs CreateDescriptorHeap"))
                return SwapChainHandle();

            rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // Create frame resources.
        {
            impl->renderTargets_.resize(params.bufferCount);

            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (auto i = 0u; i < params.bufferCount; i++) {
                auto rtv = new DX12RenderTargetView();

                auto hr = impl->swapchain_->GetBuffer(i, IID_PPV_ARGS(&rtv->texture_));

                if (CheckAPIFailed(hr, "Failed to get swap chain buffer"))
                    return SwapChainHandle();

                device_->CreateRenderTargetView(rtv->texture_.Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, rtvDescriptorSize_);

                impl->renderTargets_[i].reset(rtv);
            }
        }

        impl->vsync_ = params.vsync;

        return std::make_unique<DX12SwapChainImpl>(impl);
    }

    TextureHandle DX12::CreateTexture(const TextureParamHandle& params)
    {
        TRACE_SCOPED_DX12;

        if ((params->viewflags & static_cast<uint8_t>(EResourceViews::RV_CPU_READ)) != 0) {
            LOGE << "Textures cannot be created with EResourceViews::RV_CPU_READ";
            return TextureHandle();
        }

        auto isSRV = (params->viewflags & static_cast<uint8_t>(EResourceViews::RV_SRV)) != 0;
        auto isUAV = (params->viewflags & static_cast<uint8_t>(EResourceViews::RV_UAV)) != 0;
        auto is3d = params->depth > 1;
        auto is1d = params->height == 1;
        auto is2d = (!is3d) && (!is1d);
        auto isCube = is2d && (params->arraySize == CUBEMAP_NUM_FACES);
        auto isCubeArray = is2d && (params->arraySize > CUBEMAP_NUM_FACES) && ((params->arraySize % CUBEMAP_NUM_FACES) == 0);
        auto haveData = !params->imageDatas.empty();

        auto fmt = boost::format("Creating Texture: Size=%1%x%2%, Mips=%3% InitialData=%4%") % params->width % params->height % params->numMips % params->imageDatas.size();
        LOGD << boost::str(fmt);

        auto impl = std::make_shared<DX12TextureInternal>();

        tracker_.RegisterObject(impl);

        impl->desc_ = params;

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

        auto hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            resState,
            nullptr,
            IID_PPV_ARGS(impl->texture_.GetAddressOf()));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommittedResource"))
            return TextureHandle();

        if (haveData) {
            DX12Resource upload;

            auto numImageImpls = static_cast<uint32_t>(params->imageDatas.size());
            auto reqSize = GetRequiredIntermediateSize(impl->texture_.Get(), 0, numImageImpls);

            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            desc = CD3DX12_RESOURCE_DESC::Buffer(reqSize);

            hr = device_->CreateCommittedResource(
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

            // copy
            auto cmdList = CreateCommandList(QT_DIRECT);

            if (cmdList == nullptr) {
                return TextureHandle();
            }

            auto push = CD3DX12_RESOURCE_BARRIER::Transition(impl->texture_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

            cmdList->gfxCmdList_->ResourceBarrier(1, &push);

            UpdateSubresources(cmdList->gfxCmdList_.Get(), impl->texture_.Get(), upload.Get(), 0, 0, numImageImpls, initialData.data());

            ExecuteCommand(cmdList);

            // transition
            cmdList = CreateCommandList(QT_DIRECT);

            if (cmdList == nullptr) {
                return TextureHandle();
            }

            auto pop = CD3DX12_RESOURCE_BARRIER::Transition(impl->texture_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            cmdList->gfxCmdList_->ResourceBarrier(1, &pop);

            ExecuteCommand(cmdList);

            if (!Flush())
                return TextureHandle();
        }

        if (isSRV) {
            auto lmbd = [&](const std::shared_ptr<DX12TextureInternal>& internal, SRVHandle& target, uint32_t arrayIndex)
            {
                auto srv = new DX12ShaderResourceView(arrayIndex);

                srv->resource_ = internal;
                target.reset(srv);
            };

            if (isCubeArray) {
                lmbd(impl, impl->srvCubeArray_, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }

            if (isCube) {
                // To sample texture as cubemap
                lmbd(impl, impl->srvCube_, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

                // To sample texture as array, one for each miplevel
                impl->srvArray_.resize(params->numMips);

                for (uint32_t i = 0; i < params->numMips; ++i) {
                    lmbd(impl, impl->srvArray_[i], i);
                }

                lmbd(impl, impl->srvArrayWithMips_, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            } else if (params->arraySize > 1) {
                // one for each miplevel
                impl->srvArray_.resize(params->numMips);

                for (uint32_t i = 0; i < params->numMips; ++i) {
                    lmbd(impl, impl->srvArray_[i], i);
                }
            } else {
                lmbd(impl, impl->srvDefault_, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            }
        }

        if (isUAV) {
            impl->uav_.resize(params->numMips);

            // we have to create an UAV for each miplevel
            for (uint32_t i = 0; i < params->numMips; ++i) {
                auto uav = new DX12UnorderedAccessView(i);

                uav->resource_ = impl;
                impl->uav_[i].reset(uav);
            }
        }

        return std::make_unique<DX12TextureImpl>(impl);
    }

    bool DX12::Dispatch(const CommandHandle& cmd)
    {
        TRACE_SCOPED_DX12;

        DX12Command* dxCmd = static_cast<DX12Command*>(cmd.get());

        auto shaderHash = dxCmd->GetHashShader();

        // if the user changed the corresponding shader unbind it
        if (!dxCmd->impl_.expired() && (dxCmd->impl_.lock()->contextShaderHash_ != shaderHash)) {
            dxCmd->impl_.reset();
        }

        if (dxCmd->impl_.expired()) {
            // find the appropriate context
            auto foundContext = commandContexts_.find(shaderHash);

            if (foundContext == commandContexts_.end()) {
                auto fmt = boost::format("Dispatch error: could not find command context for shader \"%1%\". Did you forget to load the shader ?") % cmd->shader;
                LOGE << boost::str(fmt);
                return false;
            }

            dxCmd->impl_ = foundContext->second;
        }

        if (CheckWeakExpired(dxCmd->impl_))
            return false;

        // we need to check if there is a constant buffer bound
        ID3D12Resource* cbuffer = nullptr;
        uint32_t cbSize = 0;

        if (!cmd->cbufferStr.empty()) {
            auto foundHandle = poolCBSmall_.cbHandles_.find(cmd->cbufferStr);

            if (foundHandle == poolCBSmall_.cbHandles_.end()) {
                LOGEF(boost::format("Constant buffer %1% has never been set using UpdateConstantBuffer") % cmd->cbufferStr);
                return false;
            }

            cbuffer = foundHandle->second;
            cbSize = poolCBSmall_.size_;
        }

        // resource bindings for this shader
        auto foundBindings = resourceBindings_.find(cmd->shader);

        if (foundBindings == resourceBindings_.end()) {
            LOGEF(boost::format("Dispatch error: could not find resource binding table for shader \"%1%\"") % cmd->shader);
            return false;
        }

        auto& bindings = foundBindings->second;

        // look for the subcontext
        auto hash = dxCmd->GetHashBindings();
        auto context = dxCmd->impl_.lock();
        auto foundHash = context->subContexts_.find(hash);

        DX12CommandSubContext* subContext = nullptr;

        if (foundHash == context->subContexts_.end()) {
            // create and initialize subcontext
            // sampler are bound in another descriptor heap so count them out
            if (!context->CreateSubContext(device_, hash, cmd->shader, static_cast<uint32_t>(bindings.size() - cmd->ssBindings.size())))
                return false;

            subContext = &context->subContexts_[hash];
            subContext->Initialize(device_, dxCmd, bindings, cbuffer, cbSize);
        } else {
            subContext = &foundHash->second;
        }

        // At most we have 2 extra samplers
        std::array<ID3D12DescriptorHeap*, 3> descriptorHeaps;

        // order is fixed, context then samplers
        auto descriptorHeapCount = 1u;
        descriptorHeaps[0] = subContext->descriptorHeap_.Get();

        if (!cmd->ssBindings.empty()) {
            for (auto& ss : cmd->ssBindings) {
                auto dxSS = static_cast<const DX12SamplerState*>(ss.second);
                auto found = bindings.find(ss.first);

                if (found == bindings.end()) {
                    LOGEF(boost::format("DX12CommandInternal::Initialize: could not find SS binding \"%1%\"") % ss.first);
                    return false;
                }

                auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE{ dxSS->descriptorHeap_->GetCPUDescriptorHandleForHeapStart() };
                device_->CreateSampler(&dxSS->desc_, handle);

                descriptorHeaps[descriptorHeapCount++] = dxSS->descriptorHeap_.Get();
            }
        }

        // resources view are bound in the descriptor heap but we still need to transition their states before we create the views
        bool uavWholeResAll = false;
        bool srvAllNull = true;
        std::unordered_map<ID3D12Resource*, bool> uavWholeRes{ cmd->uavBindings.size() };

        for (auto& srv : cmd->srvBindings) {
            if (srv.second != nullptr) {
                srvAllNull = false;
                break;
            }
        }

        if ((cmd->srvBindings.size() == 0) || srvAllNull) {
            // only UAV access will be required for the whole resources meaning UAV read/write
            uavWholeResAll = true;
        } else {
            // we need to check if this going to be the case for the UAV requested by the user
            for (auto& uavKVP : cmd->uavBindings) {
                auto dxUAV = static_cast<const DX12UnorderedAccessView*>(uavKVP.second);
                ID3D12Resource* uavRes = nullptr;

                if (std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_)) {
                    auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();

                    uavRes = locked->buffer_.Get();
                } else {
                    auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();

                    uavRes = locked->texture_.Get();
                }

                for (auto& srvKVP : cmd->srvBindings) {
                    auto dxSRV = static_cast<const DX12ShaderResourceView*>(srvKVP.second);

                    // null SRV are allowed to mimic DX11
                    if (dxSRV == nullptr)
                        continue;

                    ID3D12Resource* srvRes = nullptr;

                    if (std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxSRV->resource_)) {
                        auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxSRV->resource_);

                        if (CheckWeakExpired(weak))
                            return false;

                        auto locked = weak.lock();

                        srvRes = locked->buffer_.Get();
                    } else {
                        auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxSRV->resource_);

                        if (CheckWeakExpired(weak))
                            return false;

                        auto locked = weak.lock();

                        srvRes = locked->texture_.Get();
                    }

                    if (uavRes == srvRes) {
                        uavWholeRes[uavRes] = false;
                    } else {
                        uavWholeRes[uavRes] = true;
                    }
                }
            }
        }

        if (cmd->uavBindings.size() > 0) {
            auto cmdListUAV = CreateCommandList(QT_DIRECT);

            if (cmdListUAV == nullptr)
                return false;

            for (auto& kvp : cmd->uavBindings) {
                auto dxUAV = static_cast<const DX12UnorderedAccessView*>(kvp.second);
                auto found = bindings.find(kvp.first);

                if (found == bindings.end()) {
                    auto fmt = boost::format("Dispatch error: could not find resource bindings for \"%1%\" in \"%2%\"") % kvp.first % cmd->shader;
                    LOGE << boost::str(fmt);
                    return false;
                }

                if (std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_)) {
                    auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();
                    auto whole = uavWholeResAll;

                    if (!whole) {
                        whole = uavWholeRes[locked->buffer_.Get()];
                    }

                    std::array<D3D12_RESOURCE_BARRIER, 1> barriers = {
                        CD3DX12_RESOURCE_BARRIER::Transition(locked->buffer_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, whole ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : dxUAV->index_),
                    };

                    cmdListUAV->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
                } else {
                    // DX12TextureInternal
                    auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();
                    auto whole = uavWholeResAll;

                    if (!whole) {
                        whole = uavWholeRes[locked->texture_.Get()];
                    }

                    std::vector<D3D12_RESOURCE_BARRIER> barriers;

                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(locked->texture_.Get()));

                    if (whole || (locked->desc_->arraySize == 1)) {
                        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(locked->texture_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, whole ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : dxUAV->index_));
                    } else {
                        for (auto i = 0u; i < locked->desc_->arraySize; ++i) {
                            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(locked->texture_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, locked->desc_->numMips * i + dxUAV->index_));
                        }
                    }

                    cmdListUAV->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
                }
            }

            ExecuteCommand(cmdListUAV);
        }

        // dispatch
        auto cmdList = CreateCommandList(QT_COMPUTE);

        if (cmdList == nullptr) {
            return false;
        }

        cmdList->gfxCmdList_->SetPipelineState(context->pipelineState_.Get());
        cmdList->gfxCmdList_->SetComputeRootSignature(context->rootSignature_.Get());

        cmdList->gfxCmdList_->SetDescriptorHeaps(descriptorHeapCount, descriptorHeaps.data());

        for (auto i = 0u; i < descriptorHeapCount; ++i) {
            cmdList->gfxCmdList_->SetComputeRootDescriptorTable(i, descriptorHeaps[i]->GetGPUDescriptorHandleForHeapStart());
        }

        cmdList->gfxCmdList_->Dispatch(cmd->dispatch[0], cmd->dispatch[1], cmd->dispatch[2]);

        ExecuteCommand(cmdList);

        // revert transition back
        if (cmd->uavBindings.size() > 0) {
            auto cmdListUAV = CreateCommandList(QT_DIRECT);

            if (cmdListUAV == nullptr)
                return false;

            for (auto& kvp : cmd->uavBindings) {
                auto dxUAV = static_cast<const DX12UnorderedAccessView*>(kvp.second);

                // bindings correctness was already checked during push so skip that
                if (std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_)) {
                    auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();
                    auto whole = uavWholeResAll;

                    if (!whole) {
                        whole = uavWholeRes[locked->buffer_.Get()];
                    }

                    std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
                        CD3DX12_RESOURCE_BARRIER::Transition(locked->buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, whole ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : dxUAV->index_),
                        CD3DX12_RESOURCE_BARRIER::UAV(locked->buffer_.Get())
                    };

                    cmdListUAV->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
                } else {
                    // DX12TextureInternal
                    auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->resource_);
                    auto locked = weak.lock();
                    auto whole = uavWholeResAll;

                    if (!whole) {
                        whole = uavWholeRes[locked->texture_.Get()];
                    }

                    std::vector<D3D12_RESOURCE_BARRIER> barriers;

                    if (whole || (locked->desc_->arraySize == 1)) {
                        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(locked->texture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, whole ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : dxUAV->index_));
                    } else {
                        for (auto i = 0u; i < locked->desc_->arraySize; ++i) {
                            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(locked->texture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, locked->desc_->numMips * i + dxUAV->index_));
                        }
                    }

                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(locked->texture_.Get()));

                    cmdListUAV->gfxCmdList_->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
                }
            }

            ExecuteCommand(cmdListUAV);
        }

        return true;
    }

    bool DX12::ExecuteCommand(CommandList* cmdList)
    {
        TRACE_SCOPED_DX12;

        cmdList->gfxCmdList_->Close();

        if (Globals::Instance().safeAndSlowDX12) {
            // for now we can execute the command list right away
            std::array<ID3D12CommandList*, 1> pCommandLists = { cmdList->gfxCmdList_.Get() };
            uint64_t fenceValue = InterlockedIncrement(&fenceValue_);
            HRESULT hr = E_FAIL;

            queues_[cmdList->type_].cmdQueue->ExecuteCommandLists(1, &pCommandLists.front());
            hr = queues_[cmdList->type_].cmdQueue->Signal(fence_.Get(), fenceValue);

            if (CheckAPIFailed(hr, "ID3D12CommandQueue::Signal"))
                return false;

            hr = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);

            if (CheckAPIFailed(hr, "ID3D12Fence::SetEventOnCompletion"))
                return false;

            WaitForSingleObject(fenceEvent_, INFINITE);

            queues_[cmdList->type_].cmdList->Reset(queues_[cmdList->type_].cmdAllocator.Get(), nullptr);
        } else {
            // put in the queue and execute when flush is called
            commands_.push_back(cmdList);
        }

        return true;
    }

    void DX12::Finalize()
    {
        TRACE_SCOPED_DX12;

        if (!commands_.empty()) {
            if (!Flush())
                throw std::exception("Finalize flush failed");
        }

        CloseHandle(fenceEvent_);

        tracker_.ReleaseObjects();

        DXGI::ReleaseDXGIFactory();
    }

    bool DX12::Flush()
    {
        TRACE_SCOPED_DX12;

        if (Globals::Instance().safeAndSlowDX12) {
            return true;
        }

        if (commands_.empty()) {
            LOGW << "Flush() was called but the command list was empty";
            return true;
        }

        std::array<ID3D12CommandList*, 1> cmdList;
        uint64_t fenceValue = std::numeric_limits<uint64_t>::max();

        auto lmbd = [&](Queue& executeQueue, Queue& waitA, Queue& waitB)
        {
            waitA.cmdQueue->Wait(fence_.Get(), fenceValue);
            waitB.cmdQueue->Wait(fence_.Get(), fenceValue);

            executeQueue.cmdQueue->ExecuteCommandLists(1, cmdList.data());

            auto hr = executeQueue.cmdQueue->Signal(fence_.Get(), fenceValue);

            if (CheckAPIFailed(hr, "ID3D12CommandQueue::Signal"))
                return false;

            return true;
        };

        for (auto& iter : commands_) {
            fenceValue = InterlockedIncrement(&fenceValue_);

            cmdList[0] = iter->gfxCmdList_.Get();

            switch (iter->type_) {
                case ninniku::QT_DIRECT:
                {
                    if (!lmbd(queues_[QT_DIRECT], queues_[QT_COPY], queues_[QT_COMPUTE]))
                        return false;
                }
                break;

                case ninniku::QT_COMPUTE:
                {
                    if (!lmbd(queues_[QT_COMPUTE], queues_[QT_COPY], queues_[QT_DIRECT]))
                        return false;
                }
                break;

                case ninniku::QT_COPY:
                {
                    if (!lmbd(queues_[QT_COPY], queues_[QT_COMPUTE], queues_[QT_DIRECT]))
                        return false;
                }
                break;

                default:
                    throw new std::exception("Invalid queue type");
                    break;
            }

            iter->~CommandList();
            poolCmd_.free(iter);
        }

        if (fenceValue != std::numeric_limits<uint64_t>::max()) {
            // we still need to wait for commands to finish running
            auto hr = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);

            if (CheckAPIFailed(hr, "ID3D12Fence::SetEventOnCompletion"))
                return false;

            WaitForSingleObject(fenceEvent_, INFINITE);
        }

        commands_.clear();

        return true;
    }

    bool DX12::Initialize()
    {
        TRACE_SCOPED_DX12;

        auto adapter = 0;

        if ((type_ & ERenderer::RENDERER_WARP) != 0)
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

        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = 1;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        auto hr = device_->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerHeap_));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap"))
            return false;

        // Create Queues
        for (auto iter = 0u; iter < QT_COUNT; iter++) {
            auto listType = QueueTypeToDX12ComandListType(static_cast<EQueueType>(iter));

            hr = device_->CreateCommandAllocator(listType, IID_PPV_ARGS(&queues_[iter].cmdAllocator));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandAllocator"))
                return false;

            D3D12_COMMAND_QUEUE_DESC queueDesc = { listType, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };

            hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queues_[iter].cmdQueue));

            if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandQueue"))
                return false;

            if (Globals::Instance().safeAndSlowDX12) {
                hr = device_->CreateCommandList(0, listType, queues_[iter].cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&queues_[iter].cmdList));

                if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList"))
                    return false;
            }
        }

        // fence
        hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateFence"))
            return false;

        // fence event
        fenceEvent_ = CreateEvent(nullptr, false, false, L"Ninniku Fence Event");
        if (fenceEvent_ == nullptr) {
            LOGE << "Failed to create fence event";
            return false;
        }

        if (!CreateSamplers())
            return false;

        // Constant buffer pool
        poolCBSmall_.size_ = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        for (auto i = 0u; i < CONSTANT_BUFFER_POOL_SIZE; ++i) {
            if (!CreateConstantBuffer(poolCBSmall_.buffers_[i], poolCBSmall_.size_))
                return false;
        }

        return true;
    }

    bool DX12::LoadShader(const std::filesystem::path& path)
    {
        TRACE_SCOPED_NAMED_DX12("ninniku::DX12::LoadShader (path)");

        if (std::filesystem::is_directory(path)) {
            return LoadShaders(path);
        } else if (path.extension() == ShaderExt) {
            auto fmt = boost::format("Loading %1%..") % path;

            LOG_INDENT_START << boost::str(fmt);

            IDxcLibrary* pLibrary = GetDXCLibrary();

            if (pLibrary == nullptr) {
                LOG_INDENT_END;
                return false;
            }

            Microsoft::WRL::ComPtr<IDxcBlobEncoding> pBlob = nullptr;

            auto hr = pLibrary->CreateBlobFromFile(ninniku::strToWStr(path.string()).c_str(), nullptr, &pBlob);

            if (CheckAPIFailed(hr, "IDxcLibrary::CreateBlobFromFile")) {
                LOG_INDENT_END;
                return false;
            }

            if (!ValidateDXCBlob(pBlob.Get(), pLibrary)) {
                LOG_INDENT_END;
                return false;
            }

            if (!LoadShader(path, pBlob.Get())) {
                LOG_INDENT_END;
                return false;
            }

            LOG_INDENT_END;
        }

        return true;
    }

    bool DX12::LoadShader(const std::string_view& name, const void* pData, const uint32_t size)
    {
        TRACE_SCOPED_NAMED_DX12("ninniku::DX12::LoadShader (string, void*, uint32_t)");

        auto fmt = boost::format("Loading %1% directly from memory..") % name;

        LOG_INDENT_START << boost::str(fmt);

        IDxcLibrary* pLibrary = GetDXCLibrary();

        if (pLibrary == nullptr) {
            LOG_INDENT_END;
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> pBlob = nullptr;

        auto hr = pLibrary->CreateBlobWithEncodingFromPinned(pData, size, 0, pBlob.GetAddressOf());

        if (CheckAPIFailed(hr, "IDxcLibrary::CreateBlobWithEncodingFromPinned")) {
            LOG_INDENT_END;
            return false;
        }

        if (!ValidateDXCBlob(pBlob.Get(), pLibrary)) {
            LOG_INDENT_END;
            return false;
        }

        if (!LoadShader(name, pBlob.Get())) {
            LOG_INDENT_END;
            return false;
        }

        LOG_INDENT_END;

        return true;
    }

    bool DX12::LoadShader(const std::filesystem::path& path, IDxcBlobEncoding* pBlob)
    {
        TRACE_SCOPED_NAMED_DX12("ninniku::DX12::LoadShader (path, IDxcBlobEncoding)");

        auto name = path.stem().string();
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
        LOGDF(boost::format("Adding CS: \"%1%\" to library") % name);
        shaders_.emplace(name, CD3DX12_SHADER_BYTECODE(pBlob->GetBufferPointer(), pBlob->GetBufferSize()));

        // Create command contexts for all the shaders we just found
        if (!CreateCommandContexts())
            return false;

        return true;
    }

    /// <summary>
    /// Load all shaders in /data
    /// </summary>
    bool DX12::LoadShaders(const std::filesystem::path& shaderPath)
    {
        TRACE_SCOPED_DX12;

        // check if directory is valid
        if (!std::filesystem::is_directory(shaderPath)) {
            auto fmt = boost::format("Failed to open directory: %1%") % shaderPath;
            LOGE << boost::str(fmt);

            return false;
        }

        // Count the number of .dxco found
        std::filesystem::directory_iterator begin(shaderPath), end;

        auto fileCounter = [&](const std::filesystem::directory_entry& d)
        {
            return (!is_directory(d.path()) && (d.path().extension() == ShaderExt));
        };

        auto numFiles = std::count_if(begin, end, fileCounter);
        auto fmt = boost::format("Found %1% compiled shaders in %2%") % numFiles % shaderPath;

        LOGD << boost::str(fmt);

        for (auto& iter : std::filesystem::recursive_directory_iterator(shaderPath)) {
            LoadShader(iter.path());
        }

        return true;
    }

    MappedResourceHandle DX12::Map(const BufferHandle& bObj)
    {
        TRACE_SCOPED_DX12;

        auto impl = static_cast<const DX12BufferImpl*>(bObj.get());

        if (CheckWeakExpired(impl->impl_))
            return MappedResourceHandle();

        // make sure data is up to date
        if (!Flush())
            return MappedResourceHandle();

        auto internal = impl->impl_.lock();
        void* data = nullptr;

        auto hr = internal->buffer_->Map(0, nullptr, &data);

        if (CheckAPIFailed(hr, "ID3D12Resource::Map"))
            return std::unique_ptr<MappedResource>();

        return std::make_unique<DX12MappedResource>(internal->buffer_, nullptr, 0, data);
    }

    MappedResourceHandle DX12::Map([[maybe_unused]] const TextureHandle& tObj, [[maybe_unused]] const uint32_t index)
    {
        throw std::exception("not implemented");
    }

    bool DX12::ParseRootSignature(const std::string_view& name, IDxcBlobEncoding* pBlob)
    {
        TRACE_SCOPED_DX12;

        DX12RootSignature rootSignature;

        auto hr = device_->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateRootSignature"))
            return false;

        LOGD << "Found a root signature";

        rootSignature->SetName(strToWStr(name).c_str());

        // for now create a root signature per shader
        rootSignatures_.emplace(name, std::move(rootSignature));

        return true;
    }

    bool DX12::ParseShaderResources(const std::string_view& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection)
    {
        TRACE_SCOPED_DX12;

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

            // if the type is a constant buffer, we want to create it a slot for it in the map
            if (bindDesc.Type == D3D_SIT_CBUFFER) {
                cBuffers_.insert(bindDesc.Name);
            }

            bindings.emplace(bindDesc.Name, bindDesc);
        }

        resourceBindings_.emplace(name, std::move(bindings));

        LOGD_INDENT_END;

        return true;
    }

    D3D12_COMMAND_LIST_TYPE  DX12::QueueTypeToDX12ComandListType(EQueueType type) const
    {
        switch (type) {
            case ninniku::QT_COMPUTE:
                return D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;

            case ninniku::QT_COPY:
                return D3D12_COMMAND_LIST_TYPE_COPY;
                break;

            case ninniku::QT_DIRECT:
                return D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;

            default:
                throw std::exception("Invalid EQueueType");
        }
    }

    bool DX12::Present(const SwapChainHandle& swapchain)
    {
        if (!Flush())
            return false;

        auto dx12sc = static_cast<const DX12SwapChainImpl*>(swapchain.get());

        if (CheckWeakExpired(dx12sc->impl_))
            return false;

        auto scInternal = dx12sc->impl_.lock();

        bool allowTearing;

        if (!CheckFeatureSupport(DF_ALLOW_TEARING, allowTearing))
            return false;

        uint32_t interval = scInternal->vsync_ ? 1 : 0;
        uint32_t flags = ((!scInternal->vsync_) && allowTearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;

        scInternal->swapchain_->Present(interval, flags);

        return true;
    }

    bool DX12::UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size)
    {
        TRACE_SCOPED_DX12;

        auto found = cBuffers_.find(name);

        if (found == cBuffers_.end()) {
            LOGEF(boost::format("Constant buffer \"%1%\" was not found in any of the shaders parsed") % name);

            return false;
        }

        // constant buffer is valid proceed with update
        if (size > poolCBSmall_.size_) {
            LOGE << "Data size is bigger than constant buffer size";
            return false;
        }

        auto poolIndex = poolCBSmall_.current_ - poolCBSmall_.lastFlush_;

        if (poolIndex >= CONSTANT_BUFFER_POOL_SIZE) {
            LOGW << "Reached CONSTANT_BUFFER_POOL_SIZE so trigerring an early flush";

            if (!Flush())
                return false;

            poolCBSmall_.lastFlush_.store(poolCBSmall_.current_);
            poolIndex = 0;
        }

        ID3D12Resource* buffer = poolCBSmall_.buffers_[poolIndex].resource_.Get();
        ID3D12Resource* upload = poolCBSmall_.buffers_[poolIndex].upload_.Get();

        // transition
        auto cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return false;
        }

        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COMMON);

        cmdList->gfxCmdList_->ResourceBarrier(1, &transition);

        ExecuteCommand(cmdList);

        // copy
        cmdList = CreateCommandList(QT_COPY);

        if (cmdList == nullptr) {
            return false;
        }

        transition = CD3DX12_RESOURCE_BARRIER::Transition(buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

        cmdList->gfxCmdList_->ResourceBarrier(1, &transition);

        D3D12_SUBRESOURCE_DATA subdata = {};
        subdata.pData = data;
        subdata.RowPitch = size;
        subdata.SlicePitch = subdata.RowPitch;

        UpdateSubresources(cmdList->gfxCmdList_.Get(), buffer, upload, 0, 0, 1, &subdata);

        ExecuteCommand(cmdList);

        // transition
        cmdList = CreateCommandList(QT_DIRECT);

        if (cmdList == nullptr) {
            return false;
        }

        transition = CD3DX12_RESOURCE_BARRIER::Transition(buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        cmdList->gfxCmdList_->ResourceBarrier(1, &transition);

        ExecuteCommand(cmdList);

        // tag the cb last update and handle
        poolCBSmall_.current_++;
        poolCBSmall_.cbHandles_[name] = buffer;

        return true;
    }
} // namespace ninniku