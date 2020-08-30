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

#include "dx12_types.h"

#include "../../../globals.h"
#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../../../utils/trace.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "pix3.h"
#pragma clang diagnostic pop

#include <d3dx12/d3dx12.h>

#include <boost/crc.hpp>

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // DX12BufferImpl
    //////////////////////////////////////////////////////////////////////////
    DX12BufferImpl::DX12BufferImpl(const std::shared_ptr<DX12BufferInternal>& impl) noexcept
        : impl_{ impl }
    {
    }

    const std::tuple<uint8_t*, uint32_t> DX12BufferImpl::GetData() const
    {
        if (CheckWeakExpired(impl_))
            return std::tuple<uint8_t*, uint32_t>();

        auto& data = impl_.lock()->data_;

        return { reinterpret_cast<uint8_t*>(data.data()), static_cast<uint32_t>(data.size() * sizeof(uint32_t)) };
    }

    const BufferParam* DX12BufferImpl::GetDesc() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->desc_.get();
    }

    const ShaderResourceView* DX12BufferImpl::GetSRV() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srv_.get();
    }

    const UnorderedAccessView* DX12BufferImpl::GetUAV() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->uav_.get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    bool DX12CommandSubContext::Initialize(const DX12Device& device, DX12ComputeCommand* cmd, const MapNameSlot& bindings, ID3D12Resource* cbuffer, uint32_t cbSize)
    {
        TRACE_SCOPED_DX12;

        if (CheckWeakExpired(cmd->impl_))
            return false;

        auto cmdImpl = cmd->impl_.lock();

        if (heapIncrementSizes_[0] == 0) {
            // increment size are vendor specific so we still need to query them once
            heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle{ descriptorHeap_->GetCPUDescriptorHandleForHeapStart() };

        // create constant buffer view, just one supported at the moment
        if (cbuffer != nullptr) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = cbuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = cbSize;

            device->CreateConstantBufferView(&cbvDesc, heapHandle);
            heapHandle.Offset(heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
        }

        // create srv bindings to the resource
        for (auto& srv : cmd->srvBindings) {
            auto found = bindings.find(srv.first);

            if (found == bindings.end()) {
                LOGEF(boost::format("DX12CommandInternal::Initialize: could not find SRV binding \"%1%\"") % srv.first);
                return false;
            }

            auto dxSRV = static_cast<const DX12ShaderResourceView*>(srv.second);

            if (dxSRV == nullptr) {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(found->second.Dimension);
                device->CreateShaderResourceView(nullptr, &srvDesc, heapHandle);
                heapHandle.Offset(heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
            } else {
                if (found->second.Type == D3D_SIT_TEXTURE) {
                    if (!std::holds_alternative<std::weak_ptr<DX12TextureInternal>>(dxSRV->resource_)) {
                        LOGEF(boost::format("SRV binding should have been a texture \"%1%\"") % found->first);
                        return false;
                    }

                    auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxSRV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();

                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srvDesc.Format = static_cast<DXGI_FORMAT>(NinnikuFormatToDXGIFormat(locked->desc_->format));

                    if (found->second.Dimension == D3D_SRV_DIMENSION_BUFFEREX) {
                        // might be dangerous is we intend those because they overlap
                        // D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE or
                        // D3D_SRV_DIMENSION_BUFFEREX
                        throw new std::exception("potential overlap");
                    }

                    srvDesc.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(found->second.Dimension);

                    switch (found->second.Dimension) {
                        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                        {
                            srvDesc.TextureCubeArray = {};
                            srvDesc.TextureCubeArray.MipLevels = locked->desc_->numMips;
                            srvDesc.TextureCubeArray.NumCubes = locked->desc_->arraySize / CUBEMAP_NUM_FACES;
                        }
                        break;

                        case D3D_SRV_DIMENSION_TEXTURECUBE:
                        {
                            srvDesc.TextureCube = {};
                            srvDesc.TextureCube.MipLevels = locked->desc_->numMips;
                        }
                        break;

                        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                        {
                            srvDesc.Texture2DArray = {};
                            srvDesc.Texture2DArray.ArraySize = locked->desc_->arraySize;

                            if (dxSRV->index_ != std::numeric_limits<uint32_t>::max()) {
                                // one slice for a mip level
                                srvDesc.Texture2DArray.MostDetailedMip = dxSRV->index_;
                                srvDesc.Texture2DArray.MipLevels = 1;
                            } else {
                                // everything mips included
                                srvDesc.Texture2DArray.MostDetailedMip = 0;
                                srvDesc.Texture2DArray.MipLevels = locked->desc_->numMips;
                            }
                        }
                        break;

                        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                        {
                            srvDesc.Texture1DArray = {};
                            srvDesc.Texture1DArray.ArraySize = locked->desc_->arraySize;
                            srvDesc.Texture1DArray.MostDetailedMip = dxSRV->index_;
                            srvDesc.Texture1DArray.MipLevels = 1;
                        }
                        break;

                        case D3D_SRV_DIMENSION_TEXTURE1D:
                        {
                            srvDesc.Texture1D = {};
                            srvDesc.Texture1D.MipLevels = locked->desc_->numMips;
                        }
                        break;

                        case D3D_SRV_DIMENSION_TEXTURE2D:
                        {
                            srvDesc.Texture2D = {};
                            srvDesc.Texture2D.MipLevels = locked->desc_->numMips;
                        }
                        break;

                        case D3D_SRV_DIMENSION_TEXTURE3D:
                        {
                            srvDesc.Texture3D = {};
                            srvDesc.Texture3D.MipLevels = locked->desc_->numMips;
                        }
                        break;

                        default:
                            throw new std::exception("Unsupported SRV binding dimension");
                    }

                    auto fmt = boost::format("%1% %2%") % found->first % dxSRV->index_;

                    locked->texture_->SetName(strToWStr(boost::str(fmt)).c_str());
                    device->CreateShaderResourceView(locked->texture_.Get(), &srvDesc, heapHandle);
                    heapHandle.Offset(heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
                } else if (found->second.Type == D3D_SIT_STRUCTURED) {
                    if (!std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxSRV->resource_)) {
                        LOGEF(boost::format("SRV binding should have been a buffer \"%1%\"") % found->first);
                        return false;
                    }

                    auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxSRV->resource_);

                    if (CheckWeakExpired(weak))
                        return false;

                    auto locked = weak.lock();

                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    srvDesc.Buffer.FirstElement = 0;
                    srvDesc.Buffer.NumElements = locked->desc_->numElements;
                    srvDesc.Buffer.StructureByteStride = locked->desc_->elementSize;
                    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

                    locked->buffer_->SetName(strToWStr(found->first).c_str());
                    device->CreateShaderResourceView(locked->buffer_.Get(), &srvDesc, heapHandle);
                    heapHandle.Offset(heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
                }
            }
        }

        // create uav bindings to the resource
        for (auto& uav : cmd->uavBindings) {
            auto dxUAV = static_cast<const DX12UnorderedAccessView*>(uav.second);
            auto found = bindings.find(uav.first);

            if (found == bindings.end()) {
                LOGEF(boost::format("DX12CommandInternal::Initialize: could not find UAV binding \"%1%\"") % uav.first);
                return false;
            }

            if (found->second.Type == D3D_SIT_UAV_RWTYPED) {
                if (!std::holds_alternative<std::weak_ptr<DX12TextureInternal>>(dxUAV->resource_)) {
                    LOGEF(boost::format("SRV binding should have been a texture \"%1%\"") % found->first);
                    return false;
                }

                auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->resource_);

                if (CheckWeakExpired(weak))
                    return false;

                auto locked = weak.lock();

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = static_cast<DXGI_FORMAT>(NinnikuFormatToDXGIFormat(locked->desc_->format));

                if (locked->desc_->arraySize > 1) {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = dxUAV->index_;
                    uavDesc.Texture2DArray.ArraySize = locked->desc_->arraySize;
                } else {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = dxUAV->index_;
                }

                //CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle{ cmdImpl->_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<int32_t>(found->second.BindPoint), _srvUAVDescriptorSize };

                auto fmt = boost::format("%1% %2%") % found->first % dxUAV->index_;

                locked->texture_->SetName(strToWStr(boost::str(fmt)).c_str());
                device->CreateUnorderedAccessView(locked->texture_.Get(), nullptr, &uavDesc, heapHandle);
                heapHandle.Offset(heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
            } else if (found->second.Type == D3D_SIT_UAV_RWSTRUCTURED) {
                if (!std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_)) {
                    LOGEF(boost::format("UAV binding should have been a buffer \"%1%\"") % found->first);
                    return false;
                }

                auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->resource_);

                if (CheckWeakExpired(weak))
                    return false;

                auto locked = weak.lock();

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.NumElements = locked->desc_->numElements;
                uavDesc.Buffer.StructureByteStride = locked->desc_->elementSize;
                uavDesc.Buffer.CounterOffsetInBytes = 0;
                uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

                locked->buffer_->SetName(strToWStr(found->first).c_str());
                device->CreateUnorderedAccessView(locked->buffer_.Get(), nullptr, &uavDesc, heapHandle);
                heapHandle.Offset(heapIncrementSizes_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12ComputeCommand
    //////////////////////////////////////////////////////////////////////////
    DX12ComputeCommandInternal::DX12ComputeCommandInternal(uint32_t shaderHash) noexcept
        : contextShaderHash_{ shaderHash }
    {
    }

    bool DX12ComputeCommandInternal::CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings)
    {
        TRACE_SCOPED_DX12;

        auto iter = subContexts_.emplace(hash, DX12CommandSubContext{});
        auto& subContext = iter.first->second;

        // Create descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = numBindings;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        auto hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&subContext.descriptorHeap_));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap"))
            return false;

        auto fmt = boost::format("%1%_%2%") % name % hash;
        subContext.descriptorHeap_->SetName(strToWStr(boost::str(fmt)).c_str());

        return true;
    }

    uint32_t DX12ComputeCommand::GetHashShader() const
    {
        TRACE_SCOPED_DX12;

        boost::crc_32_type res;

        res.process_bytes(shader.data(), shader.size());

        return res.checksum();
    }

    uint32_t DX12ComputeCommand::GetHashBindings() const
    {
        TRACE_SCOPED_DX12;

        // context is common to a shader and is created after loading them
        // but since bindings can change, we need to store various descriptor heaps and other unique items
        boost::crc_32_type res;

        res.process_bytes(cbufferStr.data(), cbufferStr.size());
        res.process_bytes(&dispatch.front(), dispatch.size() * sizeof(uint32_t));

        for (auto& srv : srvBindings) {
            res.process_bytes(srv.first.data(), srv.first.size());
            res.process_bytes(&srv.second, sizeof(ShaderResourceView*));
        }

        for (auto& uav : uavBindings) {
            res.process_bytes(uav.first.data(), uav.first.size());
            res.process_bytes(&uav.second, sizeof(UnorderedAccessView*));
        }

        for (auto& ss : ssBindings) {
            res.process_bytes(ss.first.data(), ss.first.size());
            res.process_bytes(&ss.second, sizeof(SamplerState*));
        }

        return res.checksum();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12GraphicCommand
    //////////////////////////////////////////////////////////////////////////
    DX12GraphicCommandInternal::DX12GraphicCommandInternal(uint32_t shaderHash) noexcept
        : contextShaderHash_{ shaderHash }
    {
    }

    uint32_t DX12GraphicCommand::GetHashShader() const
    {
        TRACE_SCOPED_DX12;

        boost::crc_32_type res;

        res.process_bytes(pipelineStateName.data(), pipelineStateName.size());

        return res.checksum();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////
    DX12DebugMarker::DX12DebugMarker(const std::string_view& name)
    {
        if (Globals::Instance().doCapture_) {
            // https://devblogs.microsoft.com/pix/winpixeventruntime/
            // says a ID3D12CommandList/ID3D12CommandQueue should be used but cannot find that override

            auto color = PIX_COLOR_INDEX(colorIdx_++);
            PIXBeginEvent(color, name.data());
        }
    }

    DX12DebugMarker::~DX12DebugMarker()
    {
        if (Globals::Instance().doCapture_) {
            PIXEndEvent();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12MappedResource
    //////////////////////////////////////////////////////////////////////////
    DX12MappedResource::DX12MappedResource(const DX12Resource& resource, const D3D12_RANGE* range, const uint32_t subresource, void* data) noexcept
        : resource_{ resource }
        , subresource_{ subresource }
        , range_{ range }
        , data_{ data } {
    }

    DX12MappedResource::~DX12MappedResource()
    {
        resource_->Unmap(subresource_, range_);
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12ShaderResourceView
    //////////////////////////////////////////////////////////////////////////
    DX12ShaderResourceView::DX12ShaderResourceView(uint32_t index) noexcept
        : index_{ index }
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12SwapChainImpl
    //////////////////////////////////////////////////////////////////////////
    DX12SwapChainImpl::DX12SwapChainImpl(const std::shared_ptr<DX12SwapChainInternal>& impl) noexcept
        : impl_{ impl }
    {
    }

    uint32_t DX12SwapChainImpl::GetCurrentBackBufferIndex() const
    {
        if (CheckWeakExpired(impl_))
            return -1;

        return impl_.lock()->swapchain_->GetCurrentBackBufferIndex();
    }

    const SwapchainParam* DX12SwapChainImpl::GetDesc() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->desc_.get();
    }

    const RenderTargetView* DX12SwapChainImpl::GetRT(uint32_t index) const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->renderTargets_[index].get();
    }

    uint32_t DX12SwapChainImpl::GetRTCount() const
    {
        if (CheckWeakExpired(impl_))
            return 0;

        return static_cast<uint32_t>(impl_.lock()->renderTargets_.size());
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureImpl
    //////////////////////////////////////////////////////////////////////////
    DX12TextureImpl::DX12TextureImpl(const std::shared_ptr<DX12TextureInternal>& impl) noexcept
        : impl_{ impl }
    {
    }

    const TextureParam* DX12TextureImpl::GetDesc() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->desc_.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVDefault() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvDefault_.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVCube() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvCube_.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVCubeArray() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvCubeArray_.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVArray(uint32_t index) const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvArray_[index].get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVArrayWithMips() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvArrayWithMips_.get();
    }

    const UnorderedAccessView* DX12TextureImpl::GetUAV(uint32_t index) const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->uav_[index].get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12UnorderedAccessView
    //////////////////////////////////////////////////////////////////////////
    DX12UnorderedAccessView::DX12UnorderedAccessView(uint32_t index) noexcept
        : index_{ index }
    {
    }
} // namespace ninniku