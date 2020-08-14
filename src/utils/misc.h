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

#include <ninniku/types.h>

#include <boost/format.hpp>
#include <string>

// https://blog.molecular-matters.com/2015/12/11/getting-the-type-of-a-template-argument-as-string-without-rtti/
namespace internal
{
    static const unsigned int FRONT_SIZE = sizeof("internal::GetTypeNameHelper<") - 1u;
    static const unsigned int BACK_SIZE = sizeof(">::GetTypeName") - 1u;

    template <typename T>
    struct GetTypeNameHelper
    {
        static const char* GetTypeName(void)
        {
            static const size_t size = sizeof(__FUNCTION__) - FRONT_SIZE - BACK_SIZE;
            static char typeName[size] = {};
            memcpy(typeName, __FUNCTION__ + FRONT_SIZE, size - 1u);

            return typeName;
        }
    };
}

namespace ninniku
{
    NINNIKU_API constexpr uint8_t DXGIFormatToNinnikuTF(uint32_t);
    NINNIKU_API constexpr uint32_t NinnikuTFToDXGIFormat(uint32_t);
    NINNIKU_API constexpr uint32_t DXGIFormatToNumBytes(uint32_t format);
    uint32_t Align(uint32_t uLocation, uint32_t uAlign);

    const std::wstring strToWStr(const std::string_view&);
    const std::string wstrToStr(const std::wstring&);

    bool CheckAPIFailed(HRESULT hr, const std::string_view& apiName);

    template <typename T>
    const char* GetTypeName(void)
    {
        return internal::GetTypeNameHelper<T>::GetTypeName();
    }

    template<typename T>
    bool CheckWeakExpired(const std::weak_ptr<T>& weak)
    {
        TRACE_SCOPED_UTILS;

        if (weak.expired()) {
            LOGEF(boost::format("Weak pointer to %1% is expired") % GetTypeName<T>());

            return true;
        }

        return false;
    }
} // namespace ninniku