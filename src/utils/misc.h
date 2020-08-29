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

#include "log.h"
#include "trace.h"

#include <ninniku/core/renderer/types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/format.hpp>
#pragma clang diagnostic pop

#include <string>

namespace ninniku
{
    NINNIKU_API constexpr uint8_t DXGIFormatToNinnikuTF(uint32_t);
    NINNIKU_API constexpr uint32_t NinnikuFormatToDXGIFormat(uint32_t);
    NINNIKU_API constexpr uint32_t DXGIFormatToNumBytes(uint32_t format);
    uint32_t Align(uint32_t uLocation, uint32_t uAlign);

    const std::wstring strToWStr(const std::string_view&);
    const std::string wstrToStr(const std::wstring&);

    bool CheckAPIFailed(HRESULT hr, const std::string_view& apiName);

    template<typename T>
    bool CheckWeakExpired(const std::weak_ptr<T>& weak)
    {
        TRACE_SCOPED_UTILS;

        if (weak.expired()) {
            LOGE << "Weak pointer expired..";

            return true;
        }

        return false;
    }
} // namespace ninniku