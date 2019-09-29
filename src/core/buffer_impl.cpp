// Copyright(c) 2018-2019 Kitti Vongsay
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
#include "buffer_impl.h"

#include "ninniku/dx11/DX11.h"
#include "../dx11/DX11_impl.h"
#include "../utils/log.h"

namespace ninniku
{
    void BufferImpl::InitializeFromBufferObject(DX11Handle& dx, const BufferHandle& src)
    {
        assert(src->desc->elementSize % 4 == 0);

        auto stride = src->desc->elementSize / 4;

        // allocate memory
        _data.resize(stride * src->desc->numElements);

        auto fmt = boost::format("cmftImageImpl::InitializeFromBufferObject with ElementSize=%1%, NumElements=%2%") % src->desc->elementSize % src->desc->numElements;
        LOG << boost::str(fmt);

        auto marker = dx->CreateDebugMarker("InitializeFromBufferObject");

        // create a temporary object
        auto params = src->desc->Duplicate();

        params->viewflags = RV_CPU_READ;

        auto dst = dx->CreateBuffer(params);

        CopyBufferSubresourceParam copyParams = {};

        copyParams.src = src.get();
        copyParams.dst = dst.get();

        dx->CopyBufferResource(copyParams);

        auto mapped = dx->GetImpl()->MapBuffer(dst);
        uint32_t dstPitch = static_cast<uint32_t>(_data.size() * sizeof(uint32_t));

        memcpy_s(&_data.front(), dstPitch, mapped->GetData(), std::min(dstPitch, mapped->GetRowPitch()));
    }
} // namespace ninniku