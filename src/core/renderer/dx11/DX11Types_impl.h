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

#include "ninniku/renderer/Types.h"

#include <d3d11shader.h>

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // Debug
    //////////////////////////////////////////////////////////////////////////
    using DX11Marker = Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>;

    class DX11DebugMarker : public DebugMarker
    {
        // no copy of any kind allowed
        DX11DebugMarker(const DX11DebugMarker&) = delete;
        DX11DebugMarker& operator=(DX11DebugMarker&) = delete;
        DX11DebugMarker(DX11DebugMarker&&) = delete;
        DX11DebugMarker& operator=(DX11DebugMarker&&) = delete;

    public:
        DX11DebugMarker(const DX11Marker& marker, const std::string& name);
        ~DX11DebugMarker() override;

    private:
        DX11Marker _marker;
    };
} // namespace ninniku