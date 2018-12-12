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
#include "mathUtils.h"

#include <cassert>

namespace ninniku {
bool IsPow2(uint32_t x)
{
    return ((x != 0) && !(x & (x - 1)));
}

uint32_t CountMips(uint32_t faceSize)
{
    uint32_t mipLevels = 1;

    while (faceSize > 1) {
        if (faceSize > 1)
            faceSize >>= 1;

        ++mipLevels;
    }

    return mipLevels;
}

int NearestPow2Floor(int x)
{
    int res = 1;

    while (res < x)
        res = res << 1;

    res = res >> 1;

    assert(res > 0);

    return res;
}
} // namespace ninniku