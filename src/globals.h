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

#include "ninniku/utils.h"
#include "ninniku/core/renderer/renderdevice.h"

#include <renderdoc/renderdoc_app.h>

namespace ninniku
{
    class Globals
    {
    public:
        Globals() = default;
        static Globals& Instance() { return instance_; }

        RENDERDOC_API_1_4_1* renderDocApi_ = nullptr;
        RenderDeviceHandle renderer_;

        bool bc7Quick_ : 1;
        bool doCapture_ : 1;
        bool useDebugLayer_ : 1;
        bool safeAndSlowDX12 : 1;
        bool padding_ : 4;

    private:
        static Globals instance_;
    };
} // namespace ninniku