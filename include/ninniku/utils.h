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

#pragma once

#include "export.h"

#include <stdint.h>

namespace ninniku
{
    struct NINNIKU_API NonCopyable
    {
        // no copy of any kind allowed
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(NonCopyable&) = delete;
        NonCopyable(NonCopyable&&) = delete;
        NonCopyable& operator=(NonCopyable&&) = delete;

        NonCopyable() = default;
        virtual ~NonCopyable() = default;
    };

    class NINNIKU_API NonCopyableBase
    {
        // no copy of any kind allowed
        NonCopyableBase(const NonCopyableBase&) = delete;
        NonCopyableBase& operator=(NonCopyableBase&) = delete;
        NonCopyableBase(NonCopyableBase&&) = delete;
        NonCopyableBase& operator=(NonCopyableBase&&) = delete;

    protected:
        NonCopyableBase() = default;
        virtual ~NonCopyableBase() = default;
    };

    NINNIKU_API constexpr const bool IsPow2(const uint32_t x) noexcept;
    NINNIKU_API constexpr const uint32_t CountMips(const uint32_t faceSize) noexcept;
    NINNIKU_API constexpr const int NearestPow2Floor(const int x);
    NINNIKU_API constexpr const uint32_t DXGIFormatToNinnikuTF(uint32_t);
    NINNIKU_API constexpr const uint32_t NinnikuTFToDXGIFormat(uint32_t);
} // namespace ninniku