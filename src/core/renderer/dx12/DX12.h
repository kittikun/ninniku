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

#include "../../../utils/stringMap.h"
#include "DX12Types.h"

struct IDxcBlobEncoding;
struct ID3D12ShaderReflection;

namespace ninniku {
    class DX12 final : public RenderDevice
    {
    public:
        DX12(ERenderer type);

        ERenderer GetType() const override { return _type; }
        const std::string_view& GetShaderExtension() const override { return ShaderExt; }

        void CopyBufferResource(const CopyBufferSubresourceParam& params) override;
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

        const SamplerState* GetSampler(ESamplerState sampler) const override { return _samplers[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

        // Not from RenderDevice
        std::tuple<uint32_t, uint32_t> CopyTextureSubresourceToBuffer(const CopyTextureSubresourceToBufferParam& params);
        BufferHandle CreateBuffer(const TextureParamHandle& params);
        inline ID3D12Device* GetDevice() const { return _device.Get(); }

    private:
        bool CreateCommandContexts();
        bool CreateConstantBuffer(DX12ConstantBuffer& cbuffer, const std::string_view& name, void* data, const uint32_t size);
        bool CreateDevice(int adapter);
        bool CreateSamplers();
        bool ExecuteCommand(const DX12CommandQueue& queue, const DX12GraphicsCommandList& cmdList);
        bool LoadShader(const std::filesystem::path& path, IDxcBlobEncoding* pBlob);
        bool LoadShaders(const std::filesystem::path& path);
        bool ParseRootSignature(const std::string_view& name, IDxcBlobEncoding* pBlob);
        bool ParseShaderResources(const std::string_view& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection);

    private:
        static constexpr std::string_view ShaderExt = ".dxco";
        static constexpr uint32_t MAX_DESCRIPTOR_COUNT = 32;
        ERenderer _type;
        uint8_t _padding[3];

        DX12Device _device;

        // commands and fences
        DX12CommandQueue _commandQueue;
        DX12Fence _fence;
        uint64_t volatile _fenceValue;
        volatile HANDLE _fenceEvent;

        // copy
        DX12CommandAllocator _copyCommandAllocator;
        DX12CommandQueue _copyCommandQueue;
        DX12GraphicsCommandList _copyCmdList;

        // resource transition
        DX12CommandAllocator _transitionCommandAllocator;
        DX12CommandQueue _transitionCommandQueue;
        DX12GraphicsCommandList _transitionCmdList;

        // shader related
        std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> _samplers;
        StringMap<DX12RootSignature> _rootSignatures;
        StringMap<D3D12_SHADER_BYTECODE> _shaders;
        StringMap<DX12ConstantBuffer> _cBuffers;

        StringMap<MapNameSlot> _resourceBindings;

        std::unordered_map<uint32_t, std::shared_ptr<DX12CommandInternal>> _commandContexts;

        // heap
        DX12DescriptorHeap _samplerHeap;

        // tracks allocated resources
        ObjectTracker _tracker;
    };
} // namespace ninniku
