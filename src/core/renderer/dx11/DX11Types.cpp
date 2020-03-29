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

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // DX11DebugMarker
    //////////////////////////////////////////////////////////////////////////
    DX11DebugMarker::DX11DebugMarker(const DX11Marker& marker, const std::string& name)
        : _marker{ marker }
    {
#ifdef _USE_RENDERDOC
        _marker->BeginEvent(strToWStr(name).c_str());
#endif
    }

    DX11DebugMarker::~DX11DebugMarker()
    {
#ifdef _USE_RENDERDOC
        _marker->EndEvent();
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureObject
    //////////////////////////////////////////////////////////////////////////

    ID3D11Resource* DX11TextureObject::GetResource() const
    {
        ID3D11Resource* res = nullptr;

        if (std::holds_alternative<DX11Tex2D>(texture))
            res = std::get<DX11Tex2D>(texture).Get();
        else if (std::holds_alternative<DX11Tex1D>(texture))
            res = std::get<DX11Tex1D>(texture).Get();
        else if (std::holds_alternative<DX11Tex3D>(texture))
            res = std::get<DX11Tex3D>(texture).Get();

        return res;
    }

    //////////////////////////////////////////////////////////////////////////
    // DX11MappedResource
    //////////////////////////////////////////////////////////////////////////
    DX11MappedResource::DX11MappedResource(const DX11Context& context, const TextureHandle& texObj, const uint32_t index)
        : _bufferObj{ Empty_BufferHandleDX11 }
        , _context{ context }
        , _texObj{ texObj }
        , _index{ index }
        , _mapped{}
    {
    }

    DX11MappedResource::DX11MappedResource(const DX11Context& context, const BufferHandle& bufObj)
        : _bufferObj{ bufObj }
        , _context{ context }
        , _texObj{ Empty_TextureHandle }
        , _index{ }
        , _mapped{}
    {
    }

    DX11MappedResource::~DX11MappedResource()
    {
        if (_bufferObj) {
            auto obj = static_cast<const DX11BufferObject*>(_bufferObj.get());

            _context->Unmap(obj->_buffer.Get(), 0);
        } else {
            auto obj = static_cast<const DX11TextureObject*>(_texObj.get());

            _context->Unmap(obj->GetResource(), _index);
        }
    }

    uint32_t DX11MappedResource::GetRowPitch() const
    {
        return _mapped.RowPitch;
    }
} // namespace ninniku