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
#include "../../../utils/string_set.h"
#include "../../../utils/trace.h"
#include "dx12_types.h"

#include <boost/pool/object_pool.hpp>
#include <atomic>

struct IDxcBlobEncoding;
struct ID3D12ShaderReflection;

namespace D3D12MA
{
	class Allocator;
}

namespace ninniku
{
	class DX12 final : public RenderDevice
	{
	public:
		DX12(ERenderer type);

		ERenderer GetType() const override { return type_; }
		const std::string_view& GetShaderExtension() const override { return ShaderExt; }

		bool CheckFeatureSupport(EDeviceFeature feature, bool& result) override;
		bool ClearRenderTarget(const ClearRenderTargetParam& params) override;
		bool CopyBufferResource(const CopyBufferSubresourceParam& params) override;
		std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) override;
		BufferHandle CreateBuffer(const BufferParamHandle& params) override;
		BufferHandle CreateBuffer(const BufferHandle& src) override;
		ComputeCommandHandle CreateComputeCommand() const override { return std::make_unique<DX12ComputeCommand>(); }
		DebugMarkerHandle CreateDebugMarker(const std::string_view& name) const override;
		GraphicCommandHandle CreateGraphicCommand() const override { return std::make_unique<DX12GraphicCommand>(); }
		bool CreatePipelineState(const PipelineStateParam& params) override;
		SwapChainHandle CreateSwapChain(const SwapchainParamHandle& params) override;
		TextureHandle CreateTexture(const TextureParamHandle& params) override;
		bool Dispatch(const ComputeCommandHandle& cmd) override;
		void Finalize() override;
		bool Initialize() override;
		bool LoadShader(EShaderType type, const std::filesystem::path& path) override;
		bool LoadShader(EShaderType type, const std::string_view& psName, const std::filesystem::path& path) override;
		bool LoadShader(EShaderType type, const std::string_view& name, const void* pData, const uint32_t size) override;
		MappedResourceHandle Map(const BufferHandle& bObj) override;
		MappedResourceHandle Map(const TextureHandle& tObj, const uint32_t index) override;
		bool Present(const SwapChainHandle& swapchain) override;
		void RegisterInputLayout(const InputLayoutDesc& params) override;
		bool UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size) override;

		const SamplerState* GetSampler(ESamplerState sampler) const override { return samplers_[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

		// Not from RenderDevice
		std::tuple<uint32_t, uint32_t> CopySwapChainToBuffer(const CopySwapChainToBufferParam& params);
		std::tuple<uint32_t, uint32_t> CopyTextureSubresourceToBuffer(const CopyTextureSubresourceToBufferParam& params);
		BufferHandle CreateBuffer(const TextureParamHandle& params);
		inline ID3D12Device* GetDevice() const { return device_.Get(); }

	private:
		CommandList* CreateCommandList(EQueueType type);
		bool CreateComputeCommandContext(const ComputePipelineStateParam& param);
		bool CreateConstantBuffer(DX12ConstantBuffer& cbuffer, const uint32_t size);
		bool CreateDevice(int adapter);
		bool CreateGraphicCommandContext(const GraphicPipelineStateParam& params);
		bool CreateSamplers();
		D3D12_COMMAND_LIST_TYPE QueueTypeToDX12ComandListType(EQueueType type) const;
		bool ExecuteCommand(CommandList* cmdList);
		bool Flush(EFlushType type);
		bool LoadShader(EShaderType type, const std::string_view& psName, const std::filesystem::path& path, IDxcBlobEncoding* pBlob);
		bool ParseRootSignature(const std::string_view& name, IDxcBlobEncoding* pBlob);
		bool ParseShaderResources(const std::string_view& name, uint32_t numBoundResources, ID3D12ShaderReflection* pReflection);

	private:
		static constexpr std::string_view ShaderExt = ".dxco";
		static constexpr uint32_t MAX_DESCRIPTOR_COUNT = 32;
		static constexpr uint32_t DEFAULT_COMMAND_QUEUE_SIZE = 64;

		ERenderer type_;
		DX12Device device_;

		// commands
		std::vector<CommandList*> commands_;
		std::unordered_map<uint32_t, std::shared_ptr<DX12ComputeCommandInternal>> computeCommandContexts_;
		std::unordered_map<uint32_t, std::shared_ptr<DX12GraphicCommandInternal>> gfxCommandContexts_;
		std::array<Queue, QT_COUNT> queues_;

		// fence
		DX12Fence fence_;
		uint64_t volatile fenceValue_;
		volatile HANDLE fenceEvent_;

		StringMap<std::vector<D3D12_INPUT_ELEMENT_DESC>> inputLayouts_;

		// shader related
		std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> samplers_;
		StringMap<DX12RootSignature> rootSignatures_;

		StringMap<PipelineStateShaders> psShaders_;
		StringSet cBuffers_;
		StringMap<MapNameSlot> resourceBindings_;

		// heap
		DX12DescriptorHeap samplerHeap_;

		// tracks allocated resources
		ObjectTracker tracker_;

		// Object pools
		CommandBufferPool poolCBSmall_;
		boost::object_pool<CommandList> poolCmd_;

		// for swap chain (move that into contexts like the other views)
		DX12DescriptorHeap rtvHeap_;
		uint32_t rtvDescriptorSize_;

		// DX12MemoryAllocator
		D3D12MA::Allocator* allocator_;
	};
} // namespace ninniku
