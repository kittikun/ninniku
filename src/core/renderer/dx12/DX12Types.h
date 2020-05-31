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

#include "../../../utils/objectTracker.h"
#include "../../../utils/stringMap.h"

#include <d3d12.h>
#include <d3d12shader.h>

namespace ninniku
{
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

    //////////////////////////////////////////////////////////////////////////
    // DX12BufferObject
    //////////////////////////////////////////////////////////////////////////
    struct DX12BufferInternal final : TrackedObject
    {
        DX12Resource _buffer;
        SRVHandle _srv;
        UAVHandle _uav;

        // leave data here to support update later on
        std::vector<uint32_t> _data;

        // Initial desc that was used to create the resource
        std::shared_ptr<const BufferParam> _desc;
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

        std::weak_ptr<DX12BufferInternal> _impl;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12Command
    //////////////////////////////////////////////////////////////////////////
    struct DX12CommandSubContext
    {
        bool Initialize(const DX12Device& device, struct DX12Command* cmd, const MapNameSlot& bindings, const StringMap<struct DX12ConstantBuffer>& cbuffers);

        DX12DescriptorHeap _descriptorHeap;

        static inline std::array<uint32_t, 2> _heapIncrementSizes;
    };

    struct DX12CommandInternal
    {
        DX12CommandInternal(uint32_t shaderHash) noexcept;

        DX12RootSignature rootSignature_;
        DX12PipelineState pipelineState_;
        DX12CommandAllocator cmdAllocator_;
        DX12GraphicsCommandList cmdList_;

        // user might change the bound shader so keep the last used one
        uint32_t contextShaderHash_;

        bool CreateSubContext(const DX12Device& device, uint32_t hash, const std::string_view& name, uint32_t numBindings);

        std::unordered_map<uint32_t, DX12CommandSubContext> subContexts_;
    };

    struct DX12Command final : public Command
    {
        uint32_t GetHashShader() const;
        uint32_t GetHashBindings() const;

        std::weak_ptr<DX12CommandInternal> impl_;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX12ConstantBuffer
    //////////////////////////////////////////////////////////////////////////
    struct DX12ConstantBuffer
    {
        DX12Resource resource_;
        DX12Resource upload_;
        uint32_t size_ = 0;
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

    //////////////////////////////////////////////////////////////////////////
    // DX12TextureInternal
    //////////////////////////////////////////////////////////////////////////
    struct DX12TextureInternal final : TrackedObject
    {
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
    // DX12TextureImpl
    //////////////////////////////////////////////////////////////////////////
    struct CopyTextureSubresourceToBufferParam : NonCopyable
    {
        const TextureObject* tex;
        uint32_t texFace;
        uint32_t texMip;
        const BufferObject* buffer;
    };
} // namespace ninniku