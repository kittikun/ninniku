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

#include <dxc/Support/d3dx12.h>
#include <boost/crc.hpp>

namespace ninniku {
    //////////////////////////////////////////////////////////////////////////
    // DX12BufferImpl
    //////////////////////////////////////////////////////////////////////////
    DX12BufferImpl::DX12BufferImpl(const std::shared_ptr<DX12BufferInternal>& impl) noexcept
        : _impl { impl }
    {
    }

    const std::vector<uint32_t>& DX12BufferImpl::GetData() const
    {
        return _impl.lock()->_data;
    }

    const BufferParam* DX12BufferImpl::GetDesc() const
    {
        return _impl.lock()->_desc.get();
    }

    const ShaderResourceView* DX12BufferImpl::GetSRV() const
    {
        return _impl.lock()->_srv.get();
    }

    const UnorderedAccessView* DX12BufferImpl::GetUAV() const
    {
        return _impl.lock()->_uav.get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    DX12CommandInternal::DX12CommandInternal(uint32_t shaderHash) noexcept
        : _contextShaderHash { shaderHash }
    {
    }

    bool DX12CommandInternal::CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings)
    {
        auto iter = _subContexts.emplace(hash, DX12CommandSubContext{});
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
        auto cmdImpl = cmd->_impl.lock();
        auto incrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(_descriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // samplers
        {
            //if (cmd->ssBindings.size() == 0) {
            //    D3D12_SAMPLER_DESC ssDesc = {};

            //    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(cmdImpl->_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, _srvUAVDescriptorSize);

            //    device->CreateSampler(&ssDesc, srvHandle)
            //}
        }

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
            cbvDesc.BufferLocation = foundCB->second._resource->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = foundCB->second._size;

            device->CreateConstantBufferView(&cbvDesc, heapHandle);
            heapHandle.Offset(incrSize);
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
                if (!std::holds_alternative<std::weak_ptr<DX12TextureInternal>>(dxSRV->_resource)) {
                    LOGEF(boost::format("SRV binding should have been a texture \"%1%\"") % found->first);
                    return false;
                }

                auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxSRV->_resource);
                auto locked = weak.lock();

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(locked->_desc->format));

                if (found->second.Dimension == D3D_SRV_DIMENSION_BUFFEREX) {
                    // might be dangerous is we intend those because they overlap
                    // D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE or
                    // D3D_SRV_DIMENSION_BUFFEREX
                    throw new std::exception("potential overlap");
                }

                srvDesc.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(found->second.Dimension);

                switch (found->second.Dimension) {
                    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY: {
                        srvDesc.TextureCubeArray = {};
                        srvDesc.TextureCubeArray.MipLevels = locked->_desc->numMips;
                        srvDesc.TextureCubeArray.NumCubes = locked->_desc->arraySize / CUBEMAP_NUM_FACES;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURECUBE: {
                        srvDesc.TextureCube = {};
                        srvDesc.TextureCube.MipLevels = locked->_desc->numMips;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY: {
                        srvDesc.Texture2DArray = {};

                        if (dxSRV->_index != std::numeric_limits<uint32_t>::max()) {
                            // one slice for a mip level
                            srvDesc.Texture2DArray.ArraySize = locked->_desc->arraySize;
                            srvDesc.Texture2DArray.MostDetailedMip = dxSRV->_index;
                            srvDesc.Texture2DArray.MipLevels = 1;
                        } else {
                            // everything mips included
                            srvDesc.Texture2DArray.ArraySize = locked->_desc->arraySize;
                            srvDesc.Texture2DArray.MostDetailedMip = 0;
                            srvDesc.Texture2DArray.MipLevels = locked->_desc->numMips;
                        }
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY: {
                        srvDesc.Texture1DArray = {};
                        srvDesc.Texture1DArray.ArraySize = locked->_desc->arraySize;
                        srvDesc.Texture1DArray.MostDetailedMip = dxSRV->_index;
                        srvDesc.Texture1DArray.MipLevels = 1;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE1D: {
                        srvDesc.Texture1D = {};
                        srvDesc.Texture1D.MipLevels = locked->_desc->numMips;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE2D: {
                        srvDesc.Texture2D = {};
                        srvDesc.Texture2D.MipLevels = locked->_desc->numMips;
                    }
                    break;

                    case D3D12_SRV_DIMENSION_TEXTURE3D: {
                        srvDesc.Texture3D = {};
                        srvDesc.Texture3D.MipLevels = locked->_desc->numMips;
                    }
                    break;

                    default:
                        throw new std::exception("Unsupported SRV binding dimension");
                }

                device->CreateShaderResourceView(locked->_texture.Get(), &srvDesc, heapHandle);
                heapHandle.Offset(incrSize);
            } else if (found->second.Type == D3D_SIT_STRUCTURED) {
                if (!std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxSRV->_resource)) {
                    LOGEF(boost::format("SRV binding should have been a buffer \"%1%\"") % found->first);
                    return false;
                }

                auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxSRV->_resource);
                auto locked = weak.lock();

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.NumElements = locked->_desc->numElements;
                srvDesc.Buffer.StructureByteStride = locked->_desc->elementSize;
                srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

                device->CreateShaderResourceView(locked->_buffer.Get(), &srvDesc, heapHandle);
                heapHandle.Offset(incrSize);
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
                if (!std::holds_alternative<std::weak_ptr<DX12TextureInternal>>(dxUAV->_resource)) {
                    LOGEF(boost::format("SRV binding should have been a texture \"%1%\"") % found->first);
                    return false;
                }

                auto weak = std::get<std::weak_ptr<DX12TextureInternal>>(dxUAV->_resource);
                auto locked = weak.lock();

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(locked->_desc->format));

                if (locked->_desc->arraySize > 1) {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = dxUAV->_index;
                    uavDesc.Texture2DArray.ArraySize = locked->_desc->arraySize;
                } else {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = dxUAV->_index;
                }

                //CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle{ cmdImpl->_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<int32_t>(found->second.BindPoint), _srvUAVDescriptorSize };

                device->CreateUnorderedAccessView(locked->_texture.Get(), nullptr, &uavDesc, heapHandle);
                heapHandle.Offset(incrSize);
            } else if (found->second.Type == D3D_SIT_UAV_RWSTRUCTURED) {
                if (!std::holds_alternative<std::weak_ptr<DX12BufferInternal>>(dxUAV->_resource)) {
                    LOGEF(boost::format("UAV binding should have been a buffer \"%1%\"") % found->first);
                    return false;
                }

                auto weak = std::get<std::weak_ptr<DX12BufferInternal>>(dxUAV->_resource);
                auto locked = weak.lock();

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.NumElements = locked->_desc->numElements;
                uavDesc.Buffer.StructureByteStride = locked->_desc->elementSize;
                uavDesc.Buffer.CounterOffsetInBytes = 0;
                uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

                device->CreateUnorderedAccessView(locked->_buffer.Get(), nullptr, &uavDesc, heapHandle);
                heapHandle.Offset(incrSize);
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
    DX12DebugMarker::DX12DebugMarker([[maybe_unused]]const std::string_view& name)
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
        : _resource { resource }
    , _subresource{ subresource }
    , _range{ range }
    , _data{ data } {
    }

    DX12MappedResource::~DX12MappedResource()
    {
        _resource->Unmap(_subresource, _range);
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12ShaderResourceView
    //////////////////////////////////////////////////////////////////////////
    DX12ShaderResourceView::DX12ShaderResourceView(uint32_t index) noexcept
        : _index { index }
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureImpl
    //////////////////////////////////////////////////////////////////////////
    DX12TextureImpl::DX12TextureImpl(const std::shared_ptr<DX12TextureInternal>& impl) noexcept
        : _impl { impl }
    {
    }

    const TextureParam* DX12TextureImpl::GetDesc() const
    {
        return _impl.lock()->_desc.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVDefault() const
    {
        return _impl.lock()->_srvDefault.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVCube() const
    {
        return _impl.lock()->_srvCube.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVCubeArray() const
    {
        return _impl.lock()->_srvCubeArray.get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVArray(uint32_t index) const
    {
        return _impl.lock()->_srvArray[index].get();
    }

    const ShaderResourceView* DX12TextureImpl::GetSRVArrayWithMips() const
    {
        return _impl.lock()->_srvArrayWithMips.get();
    }

    const UnorderedAccessView* DX12TextureImpl::GetUAV(uint32_t index) const
    {
        return _impl.lock()->_uav[index].get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12UnorderedAccessView
    //////////////////////////////////////////////////////////////////////////
    DX12UnorderedAccessView::DX12UnorderedAccessView(uint32_t index) noexcept
        : _index { index }
    {
    }
} // namespace ninniku