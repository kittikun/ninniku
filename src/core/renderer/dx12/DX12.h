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

#include "ninniku/core/renderer/renderdevice.h"

#include "../../../utils/string_map.h"
#include "../../../utils/trace.h"
#include "dx12_types.h"

#include <boost/pool/object_pool.hpp>

struct IDxcBlobEncoding;
struct ID3D12ShaderReflection;

namespace ninniku
{
    class DX12 final : public RenderDevice
    {
    private:
        enum EQueueType : uint8_t
        {
            QT_COMPUTE,
            QT_COPY,
            QT_TRANSITION,
            QT_COUNT
        };

        struct CommandList
        {
            EQueueType type;
            DX12GraphicsCommandList gfxCmdList;
        };

        struct Queue
        {
            DX12CommandAllocator cmdAllocator;
            DX12CommandQueue cmdQueue;

            // IF_SafeAndSlowDX12 only
            DX12GraphicsCommandList cmdList;
        };

    public:
        DX12(ERenderer type);

        ERenderer GetType() const override { return type_; }
        const std::string_view& GetShaderExtension() const override { return ShaderExt; }

        bool CheckFeatureSupport(uint32_t features) override;
        bool CopyBufferResource(const CopyBufferSubresourceParam& params) override;
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) override;
        BufferHandle CreateBuffer(const BufferParamHandle& params) override;
        BufferHandle CreateBuffer(const BufferHandle& src) override;
        CommandHandle CreateCommand() const override { return std::make_unique<DX12Command>(); }
        DebugMarkerHandle CreateDebugMarker(const std::string_view& name) const override;
        TextureHandle CreateTexture(const TextureParamHandle& params) override;
        bool Dispatch(const CommandHandle& cmd) override;
        void Finalize() override;
        bool Initialize() override;
        bool LoadShader(const std::filesystem::path& path) override;
        bool LoadShader(const std::string_view& name, const void* pData, const uint32_t size) override;
        MappedResourceHandle Map(const BufferHandle& bObj) override;
        MappedResourceHandle Map(const TextureHandle& tObj, const uint32_t index) override;
        bool UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size) override;

        const SamplerState* GetSampler(ESamplerState sampler) const override { return samplers_[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

        // Not from RenderDevice
        std::tuple<uint32_t, uint32_t> CopyTextureSubresourceToBuffer(const CopyTextureSubresourceToBufferParam& params);
        BufferHandle CreateBuffer(const TextureParamHandle& params);
        inline ID3D12Device* GetDevice() const { return device_.Get(); }

    private:
        CommandList* CreateCommandList(EQueueType type);
        bool CreateCommandContexts();
        bool CreateConstantBuffer(DX12ConstantBuffer& cbuffer, const std::string_view& name, void* data, const uint32_t size);
        bool CreateDevice(int adapter);
        bool CreateSamplers();
        D3D12_COMMAND_LIST_TYPE QueueTypeToDX12ComandListType(EQueueType type) const;
        bool ExecuteCommand(CommandList* cmdList);
        bool Flush();
        bool LoadShader(const std::filesystem::path& path, IDxcBlobEncoding* pBlob);
        bool LoadShaders(const std::filesystem::path& path);
        bool ParseRootSignature(const std::string_view& name, IDxcBlobEncoding* pBlob);
        bool ParseShaderResources(const std::string_view& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection);

    private:
        static constexpr std::string_view ShaderExt = ".dxco";
        static constexpr uint32_t MAX_DESCRIPTOR_COUNT = 32;
        static constexpr uint32_t MAX_COMMAND_QUEUE = 64;

        ERenderer type_;
        uint8_t padding_[3];

        DX12Device device_;

        // commands and fences
        DX12Fence fence_;
        uint64_t volatile fenceValue_;
        volatile HANDLE fenceEvent_;

        // IF_SafeAndSlowDX12 only
        std::array<Queue, QT_COUNT> queues_;

        // shader related
        std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> samplers_;
        StringMap<DX12RootSignature> rootSignatures_;
        StringMap<D3D12_SHADER_BYTECODE> shaders_;
        StringMap<DX12ConstantBuffer> cBuffers_;

        StringMap<MapNameSlot> resourceBindings_;

        std::unordered_map<uint32_t, std::shared_ptr<DX12CommandInternal>> commandContexts_;

        // heap
        DX12DescriptorHeap samplerHeap_;

        // tracks allocated resources
        ObjectTracker tracker_;

        // Object pools
        boost::object_pool<CommandList> poolCmd_;

        //boost::circular_buffer<const CommandList*> _commands;
        std::vector<CommandList*> _commands;
    };
} // namespace ninniku
