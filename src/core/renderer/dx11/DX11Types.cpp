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
#include "DX11Types.h"

#include "../../../utils/misc.h"

namespace ninniku {
    //////////////////////////////////////////////////////////////////////////
    // DX11BufferImpl
    //////////////////////////////////////////////////////////////////////////
    DX11BufferImpl::DX11BufferImpl(const std::shared_ptr<DX11BufferInternal>& impl) noexcept
        : _impl { impl }
    {
    }

    const std::tuple<uint8_t*, uint32_t> DX11BufferImpl::GetData() const
    {
        auto& data = _impl.lock()->_data;

        return std::make_tuple(reinterpret_cast<uint8_t*>(data.data()), static_cast<uint32_t>(data.size() * sizeof(uint32_t)));
    }

    const BufferParam* DX11BufferImpl::GetDesc() const
    {
        return _impl.lock()->_desc.get();
    }

    const ShaderResourceView* DX11BufferImpl::GetSRV() const
    {
        return _impl.lock()->_srv.get();
    }

    const UnorderedAccessView* DX11BufferImpl::GetUAV() const
    {
        return _impl.lock()->_uav.get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11DebugMarker
    //////////////////////////////////////////////////////////////////////////
    DX11DebugMarker::DX11DebugMarker(const DX11Marker& marker, [[maybe_unused]]const std::string_view& name)
        : _marker{ marker }
    {
#ifdef _DO_CAPTURE
        _marker->BeginEvent(strToWStr(name).c_str());
#endif
    }

    DX11DebugMarker::~DX11DebugMarker()
    {
#ifdef _DO_CAPTURE
        _marker->EndEvent();
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11MappedResource
    //////////////////////////////////////////////////////////////////////////
    DX11MappedResource::DX11MappedResource(const DX11Context& context, const TextureHandle& texObj, const uint32_t index)
        : _context{ context }
        , _index{ index }
        , _mapped{}
    {
        auto impl = static_cast<const DX11TextureImpl*>(texObj.get());
        auto internal = impl->_impl.lock();

        _resource = static_cast<const DX11TextureInternal*>(internal.get());
    }

    DX11MappedResource::DX11MappedResource(const DX11Context& context, const BufferHandle& bufObj)
        : _context{ context }
        , _index{}
        , _mapped{}
    {
        auto impl = static_cast<const DX11BufferImpl*>(bufObj.get());
        auto internal = impl->_impl.lock();

        _resource = static_cast<const DX11BufferInternal*>(internal.get());
    }

    DX11MappedResource::~DX11MappedResource()
    {
        if (std::holds_alternative<const DX11BufferInternal*>(_resource)) {
            auto obj = std::get<const DX11BufferInternal*>(_resource);

            _context->Unmap(obj->_buffer.Get(), 0);
        } else {
            auto obj = std::get<const DX11TextureInternal*>(_resource);

            _context->Unmap(obj->GetResource(), _index);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureInternal
    //////////////////////////////////////////////////////////////////////////
    ID3D11Resource* DX11TextureInternal::GetResource() const
    {
        ID3D11Resource* res = nullptr;

        if (std::holds_alternative<DX11Tex2D>(_texture))
            res = std::get<DX11Tex2D>(_texture).Get();
        else if (std::holds_alternative<DX11Tex1D>(_texture))
            res = std::get<DX11Tex1D>(_texture).Get();
        else if (std::holds_alternative<DX11Tex3D>(_texture))
            res = std::get<DX11Tex3D>(_texture).Get();

        return res;
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureImpl
    //////////////////////////////////////////////////////////////////////////
    DX11TextureImpl::DX11TextureImpl(const std::shared_ptr<DX11TextureInternal>& impl) noexcept
        : _impl { impl }
    {
    }

    const TextureParam* DX11TextureImpl::GetDesc() const
    {
        return _impl.lock()->_desc.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVDefault() const
    {
        return _impl.lock()->_srvDefault.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVCube() const
    {
        return _impl.lock()->_srvCube.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVCubeArray() const
    {
        return _impl.lock()->_srvCubeArray.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVArray(uint32_t index) const
    {
        return _impl.lock()->_srvArray[index].get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVArrayWithMips() const
    {
        return _impl.lock()->_srvArrayWithMips.get();
    }

    const UnorderedAccessView* DX11TextureImpl::GetUAV(uint32_t index) const
    {
        return _impl.lock()->_uav[index].get();
    }
} // namespace ninniku