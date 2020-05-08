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

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // DX12BufferImpl
    //////////////////////////////////////////////////////////////////////////
    DX12BufferImpl::DX12BufferImpl(const std::shared_ptr<DX12BufferInternal>& impl) noexcept
        : _impl{ impl }
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
    // DX12CommandContext
    //////////////////////////////////////////////////////////////////////////
    DX12CommandInternal::DX12CommandInternal(const std::string_view& name) noexcept
        : _shaderName{ name }
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////
    std::atomic<uint8_t> DX12DebugMarker::_colorIdx = 0;

    DX12DebugMarker::DX12DebugMarker([[maybe_unused]]const std::string_view& name)
    {
#ifdef _USE_RENDERDOC
        // https://devblogs.microsoft.com/pix/winpixeventruntime/
        // says a ID3D12CommandList/ID3D12CommandQueue should be used but cannot find that override

        auto color = PIX_COLOR_INDEX(_colorIdx++);
        PIXBeginEvent(color, name.data());
#endif
    }

    DX12DebugMarker::~DX12DebugMarker()
    {
#ifdef _USE_RENDERDOC
        PIXEndEvent();
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12MappedResource
    //////////////////////////////////////////////////////////////////////////
    DX12MappedResource::DX12MappedResource(const DX12Resource& resource, const D3D12_RANGE* range, const uint32_t subresource, void* data) noexcept
        : _resource{ resource }
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
        : _index{ index }
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureImpl
    //////////////////////////////////////////////////////////////////////////
    DX12TextureImpl::DX12TextureImpl(const std::shared_ptr<DX12TextureInternal>& impl) noexcept
        : _impl{ impl }
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
        : _index{ index }
    {
    }
} // namespace ninniku