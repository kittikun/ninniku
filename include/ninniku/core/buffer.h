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

#pragma once

#include "../types.h"
#include "../renderer/renderdevice.h"
#include "../renderer/types.h"

namespace ninniku
{
    class BufferImpl;

    class Buffer
    {
        // no copy of any kind allowed
        Buffer(const Buffer&) = delete;
        Buffer& operator=(Buffer&) = delete;
        Buffer(Buffer&&) = delete;
        Buffer& operator=(Buffer&&) = delete;

    public:
        NINNIKU_API Buffer();
        NINNIKU_API ~Buffer();

        NINNIKU_API const std::vector<uint32_t>& GetData() const;

        // Used when transferring data back from the GPU
        NINNIKU_API void InitializeFromBufferObject(RenderDeviceHandle& dx, const BufferHandle& src);

    private:
        std::unique_ptr<BufferImpl> _impl;
    };
} // namespace ninniku
