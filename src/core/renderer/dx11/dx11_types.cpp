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

#include "dx11_types.h"

#include "../../../globals.h"
#include "../../../utils/misc.h"

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // DX11BufferImpl
    //////////////////////////////////////////////////////////////////////////
    DX11BufferImpl::DX11BufferImpl(const std::shared_ptr<DX11BufferInternal>& impl) noexcept
        : impl_{ impl }
    {
    }

    const std::tuple<uint8_t*, uint32_t> DX11BufferImpl::GetData() const
    {
        if (CheckWeakExpired(impl_))
            return std::tuple<uint8_t*, uint32_t>();

        auto& data = impl_.lock()->data_;

        return { reinterpret_cast<uint8_t*>(data.data()), static_cast<uint32_t>(data.size() * sizeof(uint32_t)) };
    }

    const BufferParam* DX11BufferImpl::GetDesc() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->desc_.get();
    }

    const ShaderResourceView* DX11BufferImpl::GetSRV() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srv_.get();
    }

    const UnorderedAccessView* DX11BufferImpl::GetUAV() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->uav_.get();
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11DebugMarker
    //////////////////////////////////////////////////////////////////////////
    DX11DebugMarker::DX11DebugMarker(const DX11Marker& marker, [[maybe_unused]] const std::string_view& name)
        : marker_{ marker }
    {
        if (Globals::Instance().doCapture_) {
            marker_->BeginEvent(strToWStr(name).c_str());
        }
    }

    DX11DebugMarker::~DX11DebugMarker()
    {
        if (Globals::Instance().doCapture_) {
            marker_->EndEvent();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11MappedResource
    //////////////////////////////////////////////////////////////////////////
    DX11MappedResource::DX11MappedResource(const DX11Context& context, const TextureHandle& texObj, const uint32_t index)
        : context_{ context }
        , index_{ index }
        , mapped_{}
    {
        auto impl = static_cast<const DX11TextureImpl*>(texObj.get());

        if (CheckWeakExpired(impl->impl_))
            throw new std::exception("DX11MappedResource ctor");

        auto internal = impl->impl_.lock();

        resource_ = static_cast<const DX11TextureInternal*>(internal.get());
    }

    DX11MappedResource::DX11MappedResource(const DX11Context& context, const BufferHandle& bufObj)
        : context_{ context }
        , index_{}
        , mapped_{}
    {
        auto impl = static_cast<const DX11BufferImpl*>(bufObj.get());

        if (CheckWeakExpired(impl->impl_))
            throw new std::exception("DX11MappedResource ctor");

        auto internal = impl->impl_.lock();

        resource_ = static_cast<const DX11BufferInternal*>(internal.get());
    }

    DX11MappedResource::~DX11MappedResource()
    {
        if (std::holds_alternative<const DX11BufferInternal*>(resource_)) {
            auto obj = std::get<const DX11BufferInternal*>(resource_);

            context_->Unmap(obj->buffer_.Get(), 0);
        } else {
            auto obj = std::get<const DX11TextureInternal*>(resource_);

            context_->Unmap(obj->GetResource(), index_);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureInternal
    //////////////////////////////////////////////////////////////////////////
    ID3D11Resource* DX11TextureInternal::GetResource() const
    {
        ID3D11Resource* res = nullptr;

        if (std::holds_alternative<DX11Tex2D>(texture_))
            res = std::get<DX11Tex2D>(texture_).Get();
        else if (std::holds_alternative<DX11Tex1D>(texture_))
            res = std::get<DX11Tex1D>(texture_).Get();
        else if (std::holds_alternative<DX11Tex3D>(texture_))
            res = std::get<DX11Tex3D>(texture_).Get();

        return res;
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureImpl
    //////////////////////////////////////////////////////////////////////////
    DX11TextureImpl::DX11TextureImpl(const std::shared_ptr<DX11TextureInternal>& impl) noexcept
        : impl_{ impl }
    {
    }

    const TextureParam* DX11TextureImpl::GetDesc() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->desc_.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVDefault() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvDefault_.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVCube() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvCube_.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVCubeArray() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvCubeArray_.get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVArray(uint32_t index) const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvArray_[index].get();
    }

    const ShaderResourceView* DX11TextureImpl::GetSRVArrayWithMips() const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->srvArrayWithMips_.get();
    }

    const UnorderedAccessView* DX11TextureImpl::GetUAV(uint32_t index) const
    {
        if (CheckWeakExpired(impl_))
            return nullptr;

        return impl_.lock()->uav_[index].get();
    }
} // namespace ninniku