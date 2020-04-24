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
#include "DX12Types.h"

#include "../../../utils/log.h"
#include "pix3.h"

#include <comdef.h>

namespace ninniku {
    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    DX12Command::DX12Command(const DX12Device& device, const DX12CommandAllocator& commandAllocator)
    {
        // only support a single GPU for now
        // we also only support compute for now
        auto hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&_cmdList));

        if (FAILED(hr)) {
            LOGE << "CreateCommandList failed with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            throw new std::exception("CreateCommandList failed");
        }
    }

    DX12Command::~DX12Command()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////
    std::atomic<uint8_t> DX12DebugMarker::_colorIdx = 0;

    DX12DebugMarker::DX12DebugMarker(const std::string& name)
    {
#ifdef _USE_RENDERDOC
        // https://devblogs.microsoft.com/pix/winpixeventruntime/
        // says a ID3D12CommandList/ID3D12CommandQueue should be used but cannot find that override

        auto color = PIX_COLOR_INDEX(_colorIdx++);
        PIXBeginEvent(color, name.c_str());
#endif
    }

    DX12DebugMarker::~DX12DebugMarker()
    {
#ifdef _USE_RENDERDOC
        PIXEndEvent();
#endif
    }
} // namespace ninniku