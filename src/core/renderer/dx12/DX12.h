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

        void CopyBufferResource(const CopyBufferSubresourceParam& params) override;
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) const override;
        BufferHandle CreateBuffer(const BufferParamHandle& params) override;
        BufferHandle CreateBuffer(const BufferHandle& src) override;
        CommandHandle CreateCommand() const override { return std::make_unique<DX12Command>(); }
        DebugMarkerHandle CreateDebugMarker(const std::string_view& name) const override;
        TextureHandle CreateTexture(const TextureParamHandle& params) override;
        bool Dispatch(const CommandHandle& cmd) override;
        void Finalize() override;
        bool Initialize(const std::vector<std::string_view>& shaderPaths) override;
        bool LoadShader(const std::string_view& name, const void* pData, const size_t size) override;
        MappedResourceHandle Map(const BufferHandle& bObj) override;
        MappedResourceHandle Map(const TextureHandle& tObj, const uint32_t index) override;
        bool UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size) override;

        const SamplerState* GetSampler(ESamplerState sampler) const override { return _samplers[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

        ID3D12Device* GetDevice() const { return _device.Get(); }

    private:
        bool CreateDevice(int adapter);
        bool ExecuteCommand(const DX12GraphicsCommandList& cmdList);
        bool LoadShader(const std::string_view& name, IDxcBlobEncoding* pBlob);
        bool LoadShaders(const std::string_view& shaderPath);
        bool ParseRootSignature(const std::string_view& name, IDxcBlobEncoding* pBlob);
        bool ParseShaderResources(const std::string_view& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection);

    private:
        static constexpr uint32_t MAX_DESCRIPTOR_COUNT = 8;
        ERenderer _type;

        DX12Device _device;

        // commands and fences
        DX12CommandAllocator _commandAllocatorCompute;
        DX12CommandAllocator _commandAllocatorCopy;
        DX12CommandQueue _commandQueue;
        DX12Fence _fence;
        uint64_t volatile _fenceValue;
        volatile HANDLE _fenceEvent;

        // copy
        DX12GraphicsCommandList _copyCmdList;

        // shader related
        std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> _samplers;
        StringMap<DX12RootSignature> _rootSignatures;
        StringMap<D3D12_SHADER_BYTECODE> _shaders;

        using MapNameSlot = StringMap<uint32_t>;
        StringMap<MapNameSlot> _resourceBindings;

        // heap
        DX12DescriptorHeap _srvUAVHeap;
        int32_t _srvUAVIndex;
        uint32_t _srvUAVDescriptorSize;
    };
} // namespace ninniku
