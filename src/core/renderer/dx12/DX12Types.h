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

#include "ninniku/core/renderer/types.h"

#include <d3d12.h>

namespace ninniku
{
    using DX12Device = Microsoft::WRL::ComPtr<ID3D12Device>;
    using DX12RootSignature = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
    using DX12Resource = Microsoft::WRL::ComPtr<ID3D12Resource>;
    using DX12DescriptorHeap = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;

    //////////////////////////////////////////////////////////////////////////
    // DX12BufferObject
    //////////////////////////////////////////////////////////////////////////
    struct DX12BufferObject final : public BufferObject
    {
    public:
        DX12BufferObject() = default;

        // BufferObject
        const std::vector<uint32_t>& GetData() const override { return _data; }
        const ShaderResourceView* GetSRV() const override { return nullptr; }
        const UnorderedAccessView* GetUAV() const override { return nullptr; }

    public:
        DX12Resource _buffer;
        DX12Resource _srv;
        DX12Resource _uav;

        // leave data here to support update later on
        std::vector<uint32_t> _data;
    };

    static BufferHandle Empty_BufferHandleDX12;
} // namespace ninniku