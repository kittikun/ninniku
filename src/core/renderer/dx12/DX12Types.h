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

namespace ninniku {
    using DX12CommandAllocator = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
    using DX12CommandQueue = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;
    using DX12GraphicsCommandList = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
    using DX12Device = Microsoft::WRL::ComPtr<ID3D12Device>;
    using DX12Fence = Microsoft::WRL::ComPtr<ID3D12Fence>;
    using DX12PipelineState = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
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
        const ShaderResourceView* GetSRV() const override { return _srv.get(); }
        const UnorderedAccessView* GetUAV() const override { return _uav.get(); }

    public:
        DX12Resource _buffer;
        SRVHandle _srv;
        UAVHandle _uav;

        // leave data here to support update later on
        std::vector<uint32_t> _data;
    };

    static BufferHandle Empty_BufferHandleDX12;

    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    struct DX12CommandInitDesc
    {
        const DX12Device& device;
        const DX12CommandAllocator& commandAllocator;
        const D3D12_SHADER_BYTECODE& shaderCode;
        const DX12RootSignature& rootSignature;
        bool isWarp;
    };

    struct DX12Command final : public Command
    {
        bool Initialize(const DX12CommandInitDesc& initDesc);

        bool _isInitialized = false;
        DX12GraphicsCommandList _cmdList;
        DX12PipelineState _pipelineState;
        DX12RootSignature _rootSignature;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////

    struct DX12DebugMarker final : public DebugMarker
    {
    public:
        DX12DebugMarker(const std::string& name);
        ~DX12DebugMarker() override;

    private:
        static std::atomic<uint8_t> _colorIdx;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12 Shader Resources
    //////////////////////////////////////////////////////////////////////////
    struct DX12ShaderResourceView final : public ShaderResourceView
    {
    public:
        DX12Resource _resource;
    };

    struct DX12UnorderedAccessView final : public UnorderedAccessView
    {
    public:
        DX12Resource _resource;
    };

    //////////////////////////////////////////////////////////////////////////
    // GPU to CPU readback
    //////////////////////////////////////////////////////////////////////////

    struct DX12MappedResource final : public MappedResource
    {
    public:
        DX12MappedResource(const DX12Resource& resource, const D3D12_RANGE* range, const uint32_t subresource, void* data);
        ~DX12MappedResource() override;

        void* GetData() const override { return _data; }
        uint32_t GetRowPitch() const override { return 0; }

    private:
        DX12Resource _resource;
        uint32_t _subresource;
        const D3D12_RANGE* _range;
        void* _data;
    };
} // namespace ninniku