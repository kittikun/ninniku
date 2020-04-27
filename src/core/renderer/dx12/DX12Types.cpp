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
#include "../../../utils/misc.h"

#pragma warning(push)
#pragma warning(disable:4100)
#include "pix3.h"
#pragma warning(pop)

namespace ninniku {
    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    bool DX12Command::Initialize(const DX12Device& device, const DX12CommandAllocator& commandAllocator, const D3D12_SHADER_BYTECODE& shaderCode, const DX12RootSignature& rootSignature)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.CS = shaderCode;
        desc.pRootSignature = rootSignature.Get();

        auto hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_pso));

        // keep a reference on the root signature
        _rootSignature = rootSignature;

        // only support a single GPU for now
        // we also only support compute for now and don't expect any pipeline state changes for now
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocator.Get(), _pso.Get(), IID_PPV_ARGS(&_cmdList));

        if (CheckAPIFailed(hr, "ID3D12Device::CreateCommandList"))
            return false;

        _isInitialized = true;

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////
    std::atomic<uint8_t> DX12DebugMarker::_colorIdx = 0;

    DX12DebugMarker::DX12DebugMarker([[maybe_unused]]const std::string& name)
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