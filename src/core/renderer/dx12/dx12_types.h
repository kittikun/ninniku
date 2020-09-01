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

    //////////////////////////////////////////////////////////////////////////
    // CommandList
    //////////////////////////////////////////////////////////////////////////
    struct CommandList : NonCopyable
    {
        EQueueType type_;
        DX12GraphicsCommandList gfxCmdList_;
    };

    //////////////////////////////////////////////////////////////////////////
    // CopySwapChainToBufferParam
    //////////////////////////////////////////////////////////////////////////
    struct CopySwapChainToBufferParam : NonCopyable
    {
        const SwapChainObject* swapchain;
        uint32_t rtIndex;
        const BufferObject* buffer;
    };

    //////////////////////////////////////////////////////////////////////////
    // CopyTextureSubresourceToBufferParam
    //////////////////////////////////////////////////////////////////////////
    struct CopyTextureSubresourceToBufferParam : NonCopyable
    {
        const TextureObject* tex;
        uint32_t texFace;
        uint32_t texMip;
        const BufferObject* buffer;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12BufferObject
    //////////////////////////////////////////////////////////////////////////
    struct DX12BufferInternal final : TrackedObject
    {
        ~DX12BufferInternal();

        DX12Resource buffer_;
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
    struct DX12BufferImpl : public BufferObject
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
    // DX12Command / DX12CommandInternal / DX12CommandSubContext
    //////////////////////////////////////////////////////////////////////////
    struct DX12CommandSubContext
    {
        bool Initialize(const DX12Device& device, struct DX12ComputeCommand* cmd, const MapNameSlot& bindings, ID3D12Resource* cbuffer, uint32_t cbSize);

        DX12DescriptorHeap descriptorHeap_;

        static inline std::array<uint32_t, 3> heapIncrementSizes_;
    };

    struct DX12ComputeCommandInternal
    {
        DX12ComputeCommandInternal(uint32_t shaderHash) noexcept;

        DX12RootSignature rootSignature_;
        DX12PipelineState pipelineState_;

        // user might change the bound shader so keep hash so we don't rehash every time
        uint32_t contextShaderHash_;

        bool CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings);

        std::unordered_map<uint32_t, DX12CommandSubContext> subContexts_;
    };

    struct DX12ComputeCommand final : public ComputeCommand
    {
        uint32_t GetHashShader() const;
        uint32_t GetHashBindings() const;

        std::weak_ptr<DX12ComputeCommandInternal> impl_;
    };

    struct DX12GraphicCommandInternal
    {
        DX12GraphicCommandInternal(uint32_t shaderHash) noexcept;

        // user might change the bound shader so keep hash so we don't rehash every time
        uint32_t contextShaderHash_;
        DX12RootSignature rootSignature_;
        DX12PipelineState pipelineState_;
    };

    struct DX12GraphicCommand final : public GraphicCommand
    {
        DX12GraphicCommand(const std::shared_ptr<DX12>& device);

        uint32_t GetHashShader() const;

        bool ClearRenderTarget(const ClearRenderTargetParam& params) const override;
        bool IASetPrimitiveTopology(EPrimitiveTopology topology) const override;
        bool IASetVertexBuffers(const SetVertexBuffersParam& params) const override;

        std::weak_ptr<DX12GraphicCommandInternal> impl_;

        // should probably own its own command list instead of locking the device each time
        std::weak_ptr<DX12> device_;
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
    struct DX12DebugMarker final : public DebugMarker
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
    struct DX12MappedResource final : public MappedResource
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
    // DX12SamplerState
    //////////////////////////////////////////////////////////////////////////
    struct DX12SamplerState final : public SamplerState
    {
    public:
        DX12DescriptorHeap descriptorHeap_;
        D3D12_SAMPLER_DESC desc_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12 Shader Resources
    //////////////////////////////////////////////////////////////////////////
    struct DX12RenderTargetView final : public RenderTargetView
    {
        DX12Resource texture_;
    };

    struct DX12ShaderResourceView final : public ShaderResourceView
    {
        DX12ShaderResourceView(uint32_t index) noexcept;

        std::variant<std::weak_ptr<DX12BufferInternal>, std::weak_ptr<struct DX12TextureInternal>> resource_;

        // only when array, std::numeric_limits<uint32_t>::max() otherwise
        uint32_t index_;
    };

    struct DX12UnorderedAccessView final : public UnorderedAccessView
    {
        DX12UnorderedAccessView(uint32_t index) noexcept;

        std::variant<std::weak_ptr<DX12BufferInternal>, std::weak_ptr<struct DX12TextureInternal>> resource_;

        // only when array, std::numeric_limits<uint32_t>::max() otherwise
        uint32_t index_;
    };

    struct DX12VertexBufferView final : public VertexBufferView
    {
        D3D12_VERTEX_BUFFER_VIEW view_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12SwapChainInternal
    //////////////////////////////////////////////////////////////////////////
    struct DX12SwapChainInternal final : TrackedObject
    {
        DXGISwapChain swapchain_;
        std::vector<RTVHandle> renderTargets_;
        bool vsync_;

        // Initial desc that was used to create the resource
        std::shared_ptr<const SwapchainParam> desc_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12SwapChainImpl
    //////////////////////////////////////////////////////////////////////////
    struct DX12SwapChainImpl final : public SwapChainObject
    {
        DX12SwapChainImpl(const std::shared_ptr<DX12SwapChainInternal>& impl) noexcept;

        uint32_t GetCurrentBackBufferIndex() const override;
        const SwapchainParam* GetDesc() const override;
        const RenderTargetView* GetRT(uint32_t index) const override;
        uint32_t GetRTCount() const override;

        std::weak_ptr<DX12SwapChainInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureInternal
    //////////////////////////////////////////////////////////////////////////
    struct DX12TextureInternal final : TrackedObject
    {
        ~DX12TextureInternal();

        DX12Resource texture_;

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
    struct DX12TextureImpl final : public TextureObject
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
    struct PipelineStateShaders
    {
        std::array<D3D12_SHADER_BYTECODE, ST_Count> shaders_;
    };

    //////////////////////////////////////////////////////////////////////////
    // Queue
    //////////////////////////////////////////////////////////////////////////
    struct Queue : NonCopyable
    {
        DX12CommandAllocator cmdAllocator;
        DX12CommandQueue cmdQueue;

        // IF_SafeAndSlowDX12 only
        DX12GraphicsCommandList cmdList;
    };
} // namespace ninniku