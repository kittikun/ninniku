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
#include "DX12Types.h"

#include "../../../utils/log.h"
#include "../../../utils/misc.h"

#pragma warning(push)
#pragma warning(disable:4100)
#include "pix3.h"
#pragma warning(pop)

#include <d3dx12/d3dx12.h>
#include <boost/crc.hpp>

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // DX12BufferImpl
    //////////////////////////////////////////////////////////////////////////
    DX12BufferImpl::DX12BufferImpl(const std::shared_ptr<DX12BufferInternal>& impl) noexcept
        : _impl{ impl }
    {
    }

    const std::tuple<uint8_t*, uint32_t> DX12BufferImpl::GetData() const
    {
        if (CheckWeakExpired(_impl))
            return std::tuple<uint8_t*, uint32_t>();

        auto& data = _impl.lock()->_data;

        return std::make_tuple(reinterpret_cast<uint8_t*>(data.data()), static_cast<uint32_t>(data.size() * sizeof(uint32_t)));
    }

    const BufferParam* DX12BufferImpl::GetDesc() const
    {
        if (CheckWeakExpired(_impl))
            return nullptr;

        return _impl.lock()->_desc.get();
    }

    const ShaderResourceView* DX12BufferImpl::GetSRV() const
    {
        if (CheckWeakExpired(_impl))
            return nullptr;

        return _impl.lock()->_srv.get();
    }

    const UnorderedAccessView* DX12BufferImpl::GetUAV() const
    {
        if (CheckWeakExpired(_impl))
            return nullptr;

        return _impl.lock()->_uav.get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    DX12CommandInternal::DX12CommandInternal(uint32_t shaderHash) noexcept
        : contextShaderHash_{ shaderHash }
    {
    }

    bool DX12CommandInternal::CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings)
    {
        auto iter = subContexts_.emplace(hash, DX12CommandSubContext{});
        auto& subContext = iter.first->second;

        // Create descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = numBindings;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        auto hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&subContext._descriptorHeap));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateDescriptorHeap"))
            return false;

        auto fmt = boost::format("%1%_%2%") % name % hash;
        subContext._descriptorHeap->SetName(strToWStr(boost::str(fmt)).c_str());

        return true;
    }

    bool DX12CommandSubContext::Initialize(const DX12Device& device, DX12Command* cmd, const MapNameSlot& bindings, const StringMap<DX12ConstantBuffer>& cbuffers)
    {
        if (CheckWeakExpired(cmd->impl_))
            return false;

        auto cmdImpl = cmd->impl_.lock();

        if (_heapIncrementSizes[0] == 0) {
            // increment size are fixed per hardware but we still need to query them
            _heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            _heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle{ _descriptorHeap->GetCPUDescriptorHandleForHeapStart() };

        // create constant buffer view, just one supported at the moment
        if (!cmd->cbufferStr.empty()) {
            auto found = bindings.find(cmd->cbufferStr);

            if (found == bindings.end()) {
                LOGEF(boost::format("DX12CommandInternal::Initialize: could not find constant buffer binding \"%1%\"") % cmd->cbufferStr);
                return false;
            }

            auto foundCB = cbuffers.find(cmd->cbufferStr);

            if (foundCB == cbuffers.end()) {
                LOGEF(boost::format("Constant buffer \"%1%\" was not found in any of the shaders parsed") % cmd->cbufferStr);
                return false;
            }

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = foundCB->second.resource_->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = foundCB->second.size_;

            device->CreateConstantBufferView(&cbvDesc, heapHandle);
            heapHandle.Offset(_heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
        }

        // create srv bindings to the resource
        for (auto& srv : cmd->srvBindings) {
            auto dxSRV = static_cast<const DX12ShaderResourceView*>(srv.second);
            auto found = bindings.find(srv.first);

            if (found == bindings.end()) {
                LOGEF(boost::format("DX12CommandInternal::Initialize: could not find SRV binding \"%1%\"") % srv.first);
                return false;
            }

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
                srvDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(locked->desc_->format));

                if (found->second.Dimension == D3D_SRV_DIMENSION_BUFFEREX) {
                    // might be dangerous is we intend those because they overlap
                    // D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE or
                    // D3D_SRV_DIMENSION_BUFFEREX
                    throw new std::exception("potential overlap");
                }

                srvDesc.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(found->second.Dimension);

                switch (found->second.Dimension) {
                    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                    {
                        srvDesc.TextureCubeArray = {};
                        srvDesc.TextureCubeArray.MipLevels = locked->desc_->numMips;
                        srvDesc.TextureCubeArray.NumCubes = locked->desc_->arraySize / CUBEMAP_NUM_FACES;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURECUBE:
                    {
                        srvDesc.TextureCube = {};
                        srvDesc.TextureCube.MipLevels = locked->desc_->numMips;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
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

                    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                    {
                        srvDesc.Texture1DArray = {};
                        srvDesc.Texture1DArray.ArraySize = locked->desc_->arraySize;
                        srvDesc.Texture1DArray.MostDetailedMip = dxSRV->index_;
                        srvDesc.Texture1DArray.MipLevels = 1;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE1D:
                    {
                        srvDesc.Texture1D = {};
                        srvDesc.Texture1D.MipLevels = locked->desc_->numMips;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE2D:
                    {
                        srvDesc.Texture2D = {};
                        srvDesc.Texture2D.MipLevels = locked->desc_->numMips;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE3D:
                    {
                        srvDesc.Texture3D = {};
                        srvDesc.Texture3D.MipLevels = locked->desc_->numMips;
                    }
                    break;

                    default:
                        throw new std::exception("Unsupported SRV binding dimension");
                }

                locked->texture_->SetName(strToWStr(found->first).c_str());
                device->CreateShaderResourceView(locked->texture_.Get(), &srvDesc, heapHandle);
                heapHandle.Offset(_heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
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
                srvDesc.Buffer.NumElements = locked->_desc->numElements;
                srvDesc.Buffer.StructureByteStride = locked->_desc->elementSize;
                srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

                locked->_buffer->SetName(strToWStr(found->first).c_str());
                device->CreateShaderResourceView(locked->_buffer.Get(), &srvDesc, heapHandle);
                heapHandle.Offset(_heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
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
                uavDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(locked->desc_->format));

                if (locked->desc_->arraySize > 1) {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = dxUAV->index_;
                    uavDesc.Texture2DArray.ArraySize = locked->desc_->arraySize;
                } else {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = dxUAV->index_;
                }

                //CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle{ cmdImpl->_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<int32_t>(found->second.BindPoint), _srvUAVDescriptorSize };

                locked->texture_->SetName(strToWStr(found->first).c_str());
                device->CreateUnorderedAccessView(locked->texture_.Get(), nullptr, &uavDesc, heapHandle);
                heapHandle.Offset(_heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
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
                uavDesc.Buffer.NumElements = locked->_desc->numElements;
                uavDesc.Buffer.StructureByteStride = locked->_desc->elementSize;
                uavDesc.Buffer.CounterOffsetInBytes = 0;
                uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

                locked->_buffer->SetName(strToWStr(found->first).c_str());
                device->CreateUnorderedAccessView(locked->_buffer.Get(), nullptr, &uavDesc, heapHandle);
                heapHandle.Offset(_heapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
            }
        }

        return true;
    }

    uint32_t DX12Command::GetHashShader() const
    {
        boost::crc_32_type res;

        res.process_bytes(shader.data(), shader.size());

        return res.checksum();
    }

    uint32_t DX12Command::GetHashBindings() const
    {
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
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////
    DX12DebugMarker::DX12DebugMarker([[maybe_unused]] const std::string_view& name)
    {
#ifdef _DO_CAPTURE
        // https://devblogs.microsoft.com/pix/winpixeventruntime/
        // says a ID3D12CommandList/ID3D12CommandQueue should be used but cannot find that override

        auto color = PIX_COLOR_INDEX(_colorIdx++);
        PIXBeginEvent(color, name.data());
#endif
    }

    DX12DebugMarker::~DX12DebugMarker()
    {
#ifdef _DO_CAPTURE
        PIXEndEvent();
#endif
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