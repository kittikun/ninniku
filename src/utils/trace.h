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

#ifndef TRACY_ENABLE

#define TRACE_SCOPED_DX11
#define TRACE_SCOPED_NAMED_DX11(X)

#define TRACE_SCOPED_DX12
#define TRACE_SCOPED_NAMED_DX12(X)

#define TRACE_SCOPED_UTILS

#else

#include <Tracy.hpp>

enum ETraceCategories
{
    TC_DX11 = 1 << 0,
    TC_DX12 = 1 << 1,
    TC_UTILS = 1 << 2
};

#define TRACE_CATEGORIES (TC_DX11 | TC_DX12 | TC_UTILS)

#define TRACE_SCOPED_DX11 ZoneNamed(__tracy, TRACE_CATEGORIES & TC_DX11);
#define TRACE_SCOPED_NAMED_DX11(X) ZoneNamedN(__tracy, X, TRACE_CATEGORIES & TC_DX11);

#define TRACE_SCOPED_DX12 ZoneNamed(__tracy, TRACE_CATEGORIES & TC_DX12);
#define TRACE_SCOPED_NAMED_DX12(X) ZoneNamedN(__tracy, X, TRACE_CATEGORIES & TC_DX12);

#define TRACE_SCOPED_UTILS ZoneNamed(__tracy, TRACE_CATEGORIES & TC_UTILS);

#endif