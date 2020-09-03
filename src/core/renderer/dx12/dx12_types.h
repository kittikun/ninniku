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

#include "../../../utils/object_tracker.h"
#include "../../../utils/string_map.h"
#include "../dxgi.h"

#include <d3dx12/d3dx12.h>
#include <wrl/client.h>
#include <atomic>
#include <d3d12.h>
#include <d3d12shader.h>

#include <string_view>
#include <variant>

namespace D3D12MA
{
    class Allocation;
}

namespace ninniku
{
    class DX12;

    using DX12CommandAllocator = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
    using DX12CommandQueue = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;
    using DX12GraphicsCommandList = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
    using DX12Device = Microsoft::WRL::ComPtr<ID3D12Device>;
    using DX12Fence = Microsoft::WRL::ComPtr<ID3D12Fence>;
    using DX12PipelineState = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
    using DX12RootSignature = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
    using DX12Resource = Microsoft::WRL::ComPtr<ID3D12Resource>;
    using DX12DescriptorHeap = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;
    using MapNameSlot = StringMap<D3D12_SHADER_INPUT_BIND_DESC>;

    enum EFlushType : uint8_t
    {
        FT_DEFAULT,         // Execute then wait
        FT_EXECUTE_ONLY,
        FT_WAIT_ONLY        // Usually used by Present
    };

    enum EQueueType : uint8_t
    {
        QT_COMPUTE,
        QT_COPY,
        QT_DIRECT,
        QT_COUNT
    };

    static constexpr uint32_t CONSTANT_BUFFER_POOL_SIZE = 32;
    static std::array<uint32_t, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> HeapIncrementSizes;

    void QueryDescriptorHandleIncrementSizes(const DX12Device& device);

    struct DX12TrackedObject : TrackedObject
    {
        DX12Resource resource_;

        D3D12_RESOURCE_STATES stateCurrent;
        D3D12_RESOURCE_STATES statePrevious;

        CD3DX12_RESOURCE_BARRIER TransitionTo(D3D12_RESOURCE_STATES to, uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) noexcept;
        CD3DX12_RESOURCE_BARRIER TransitionBack(uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) noexcept;
    };

    //////////////////////////////////////////////////////////////////////////
    // CommandList
    //////////////////////////////////////////////////////////////////////////
    struct CommandList final : NonCopyable
    {
        EQueueType type_;
        DX12GraphicsCommandList gfxCmdList_;
        bool closed = false;
    };

    //////////////////////////////////////////////////////////////////////////
    // CopySwapChainToBufferParam
    //////////////////////////////////////////////////////////////////////////
    struct CopySwapChainToBufferParam final : NonCopyable
    {
        const SwapChainObject* swapchain;
        uint32_t rtIndex;
        const BufferObject* buffer;
    };

    //////////////////////////////////////////////////////////////////////////
    // CopyTextureSubresourceToBufferParam
    //////////////////////////////////////////////////////////////////////////
    struct CopyTextureSubresourceToBufferParam final : NonCopyable
    {
        const TextureObject* tex;
        uint32_t texFace;
        uint32_t texMip;
        const BufferObject* buffer;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12BufferObject
    //////////////////////////////////////////////////////////////////////////
    struct DX12BufferInternal final : DX12TrackedObject
    {
        ~DX12BufferInternal();

        SRVHandle srv_;
        UAVHandle uav_;
        VBVHandle vbv_;

        // leave data here to support update later on
        std::vector<uint32_t> data_;

        // Initial desc that was used to create the resource
        std::shared_ptr<const BufferParam> desc_;

        // D3D12MemoryAllocator
        D3D12MA::Allocation* allocation_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12BufferImpl
    //////////////////////////////////////////////////////////////////////////
    struct DX12BufferImpl final : BufferObject
    {
        DX12BufferImpl(const std::shared_ptr<DX12BufferInternal>& impl) noexcept;

        const std::tuple<uint8_t*, uint32_t> GetData() const override;
        const BufferParam* GetDesc() const override;
        const ShaderResourceView* GetSRV() const override;
        const UnorderedAccessView* GetUAV() const override;
        const VertexBufferView* GetVBV() const override;

        std::weak_ptr<DX12BufferInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12ComputeCommand / DX12ComputeCommandInternal / DX12CommandSubContext
    //////////////////////////////////////////////////////////////////////////
    struct DX12ComputeCommandSubContext
    {
        bool Initialize(const DX12Device& device, struct DX12ComputeCommand* cmd, const MapNameSlot& bindings, ID3D12Resource* cbuffer, uint32_t cbSize);

        DX12DescriptorHeap descriptorHeap_;
    };

    struct DX12ComputeCommandInternal
    {
        DX12ComputeCommandInternal(uint32_t shaderHash) noexcept;

        DX12RootSignature rootSignature_;
        DX12PipelineState pipelineState_;

        // user might change the bound shader so keep hash so we don't rehash every time
        uint32_t contextShaderHash_;

        bool CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings);

        std::unordered_map<uint32_t, DX12ComputeCommandSubContext> subContexts_;
    };

    struct DX12ComputeCommand final : ComputeCommand
    {
        uint32_t GetHashShader() const;
        uint32_t GetHashBindings() const;

        std::weak_ptr<DX12ComputeCommandInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12GraphicCommand / DX12GraphicCommandInternal / DX12GraphicCommandSubContext
    //////////////////////////////////////////////////////////////////////////
    struct DX12GraphicCommandSubContext
    {
        bool Initialize(const DX12Device& device, struct DX12ComputeCommand* cmd, const MapNameSlot& bindings, ID3D12Resource* cbuffer, uint32_t cbSize);

        DX12DescriptorHeap descriptorHeap_;
    };

    struct DX12GraphicCommandInternal
    {
        DX12GraphicCommandInternal(uint32_t shaderHash) noexcept;

        // user might change the bound shader so keep hash so we don't rehash every time
        uint32_t contextShaderHash_;
        DX12CommandAllocator allocator_;
        DX12GraphicsCommandList cmdList_;
        DX12RootSignature rootSignature_;
        DX12PipelineState pipelineState_;

        bool CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings);

        std::unordered_map<uint32_t, DX12GraphicCommandSubContext> subContexts_;
    };

    struct DX12GraphicCommand final : GraphicCommand
    {
        uint32_t GetHashShader() const;

        bool ClearRenderTarget(const ClearRenderTargetParam& params) const override;
        bool IASetPrimitiveTopology(EPrimitiveTopology topology) const override;
        bool IASetVertexBuffers(const SetVertexBuffersParam& params) const override;

        std::weak_ptr<DX12GraphicCommandInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12ConstantBuffer
    //////////////////////////////////////////////////////////////////////////
    struct DX12ConstantBuffer
    {
        ~DX12ConstantBuffer();

        DX12Resource resource_;
        DX12Resource upload_;

        // D3D12MemoryAllocator
        std::array<D3D12MA::Allocation*, 2> allocations_;
    };

    struct CommandBufferPool
    {
        std::array<DX12ConstantBuffer, CONSTANT_BUFFER_POOL_SIZE> buffers_;
        uint32_t size_;

        // pool also track buffer usage, might want to move that somewhere else
        StringMap<ID3D12Resource*> cbHandles_;

        std::atomic<uint32_t> lastFlush_ = 0;
        std::atomic<uint32_t> current_ = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12DebugMarker
    //////////////////////////////////////////////////////////////////////////
    struct DX12DebugMarker final : DebugMarker
    {
    public:
        DX12DebugMarker(const std::string_view& name);
        ~DX12DebugMarker() override;

    private:
        static inline std::atomic<uint8_t> colorIdx_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12MappedResource
    //////////////////////////////////////////////////////////////////////////
    struct DX12MappedResource final : MappedResource
    {
    public:
        DX12MappedResource(const DX12Resource& resource, const D3D12_RANGE* range, const uint32_t subresource, void* data) noexcept;
        ~DX12MappedResource() override;

        void* GetData() const override { return data_; }

    private:
        DX12Resource resource_;
        uint32_t subresource_;
        const D3D12_RANGE* range_;
        void* data_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12RenderTarget
    //////////////////////////////////////////////////////////////////////////
    struct DX12RenderTargetInteral final : DX12TrackedObject
    {
        DX12DescriptorHeap descriptorHeap_;
    };

    struct DX12RenderTarget final : RenderTargetObject
    {
        std::weak_ptr<DX12RenderTargetInteral> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12SamplerState
    //////////////////////////////////////////////////////////////////////////
    struct DX12SamplerState final : SamplerState
    {
    public:
        DX12DescriptorHeap descriptorHeap_;
        D3D12_SAMPLER_DESC desc_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12 Shader Resources
    //////////////////////////////////////////////////////////////////////////
    struct DX12ShaderResourceView final : ShaderResourceView
    {
        DX12ShaderResourceView(uint32_t index) noexcept;

        std::variant<std::weak_ptr<DX12BufferInternal>, std::weak_ptr<struct DX12TextureInternal>> resource_;

        // only when array, std::numeric_limits<uint32_t>::max() otherwise
        uint32_t index_;
    };

    struct DX12UnorderedAccessView final : UnorderedAccessView
    {
        DX12UnorderedAccessView(uint32_t index) noexcept;

        std::variant<std::weak_ptr<DX12BufferInternal>, std::weak_ptr<struct DX12TextureInternal>> resource_;

        // only when array, std::numeric_limits<uint32_t>::max() otherwise
        uint32_t index_;
    };

    struct DX12VertexBufferView final : VertexBufferView
    {
        D3D12_VERTEX_BUFFER_VIEW view_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12SwapChain
    //////////////////////////////////////////////////////////////////////////
    struct DX12SwapChainInternal final : TrackedObject
    {
        DXGISwapChain swapchain_;
        std::vector<RenderTargetHandle> renderTargets_;
        bool vsync_;

        // Initial desc that was used to create the resource
        std::shared_ptr<const SwapchainParam> desc_;
    };

    struct DX12SwapChainImpl final : SwapChainObject
    {
        DX12SwapChainImpl(const std::shared_ptr<DX12SwapChainInternal>& impl) noexcept;

        uint32_t GetCurrentBackBufferIndex() const override;
        const SwapchainParam* GetDesc() const override;
        const RenderTargetObject* GetRT(uint32_t index) const override;
        uint32_t GetRTCount() const override;

        std::weak_ptr<DX12SwapChainInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureInternal
    //////////////////////////////////////////////////////////////////////////
    struct DX12TextureInternal final : DX12TrackedObject
    {
        ~DX12TextureInternal();

        SRVHandle srvDefault_;

        // D3D_SRV_DIMENSION_TEXTURECUBE
        SRVHandle srvCube_;

        // D3D_SRV_DIMENSION_TEXTURECUBEARRAY
        SRVHandle srvCubeArray_;

        // D3D_SRV_DIMENSION_TEXTURE2DARRAY per mip level
        std::vector<SRVHandle> srvArray_;

        SRVHandle srvArrayWithMips_;

        // One D3D11_TEX2D_ARRAY_UAV per mip level
        std::vector<UAVHandle> uav_;

        // Initial desc that was used to create the resource
        std::shared_ptr<const TextureParam> desc_;

        // D3D12MemoryAllocator
        D3D12MA::Allocation* allocation_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureImpl
    //////////////////////////////////////////////////////////////////////////
    struct DX12TextureImpl final : TextureObject
    {
        DX12TextureImpl(const std::shared_ptr<DX12TextureInternal>& impl) noexcept;

        const TextureParam* GetDesc() const override;
        const ShaderResourceView* GetSRVDefault() const override;
        const ShaderResourceView* GetSRVCube() const override;
        const ShaderResourceView* GetSRVCubeArray() const override;
        const ShaderResourceView* GetSRVArray(uint32_t index) const override;
        const ShaderResourceView* GetSRVArrayWithMips() const override;
        const UnorderedAccessView* GetUAV(uint32_t index) const override;

        std::weak_ptr<DX12TextureInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // PipelineState
    //////////////////////////////////////////////////////////////////////////
    struct PipelineStateShaders final
    {
        std::array<D3D12_SHADER_BYTECODE, ST_Count> shaders_;
    };

    //////////////////////////////////////////////////////////////////////////
    // Queue
    //////////////////////////////////////////////////////////////////////////
    struct Queue final : NonCopyable
    {
        DX12CommandAllocator cmdAllocator;
        DX12CommandQueue cmdQueue;

        // IF_SafeAndSlowDX12 only
        DX12GraphicsCommandList cmdList;
    };
} // namespace ninniku