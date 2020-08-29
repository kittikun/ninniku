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
#include "../dxgi.h"
#include "dx11_types.h"

struct ID3D11ShaderReflection;

namespace ninniku
{
    class DX11 final : public RenderDevice
    {
    public:
        DX11(ERenderer type);

        // RenderDevice
        ERenderer GetType() const override { return type_; }
        const std::string_view& GetShaderExtension() const override { return ShaderExt; }

        bool CheckFeatureSupport(EDeviceFeature feature, bool& result) override;
        bool ClearRenderTarget(const ClearRenderTargetParam& params) override;
        bool CopyBufferResource(const CopyBufferSubresourceParam& params) override;
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) override;
        BufferHandle CreateBuffer(const BufferParamHandle& params) override;
        BufferHandle CreateBuffer(const BufferHandle& src) override;
        ComputeCommandHandle CreateComputeCommand() const override { return std::make_unique<ComputeCommand>(); }
        DebugMarkerHandle CreateDebugMarker(const std::string_view& name) const override;
        GraphicCommandHandle CreateGraphicCommand() const override { return std::make_unique<GraphicCommand>(); }
        bool CreatePipelineState(const PipelineStateParam& params) override;
        TextureHandle CreateTexture(const TextureParamHandle& params) override;
        SwapChainHandle CreateSwapChain(const SwapchainParamHandle& params) override;
        bool Dispatch(const ComputeCommandHandle& cmd) override;
        void Finalize() override;
        bool Initialize() override;
        bool LoadShader(EShaderType type, const std::filesystem::path& path) override;
        bool LoadShader(EShaderType type, const std::string_view& psName, const std::filesystem::path& path) override;
        bool LoadShader(EShaderType type, const std::string_view& name, const void* pData, const uint32_t size) override;
        MappedResourceHandle Map(const BufferHandle& bObj) override;
        MappedResourceHandle Map(const TextureHandle& tObj, const uint32_t index) override;
        bool Present(const SwapChainHandle&) override;
        bool UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size) override;

        const SamplerState* GetSampler(ESamplerState sampler) const override { return samplers_[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

        // Not from RenderDevice
        inline ID3D11Device* GetDevice() const { return device_.Get(); }

    private:
        bool CreateDevice(int adapter, ID3D11Device** pDevice);
        std::string_view DxSRVDimensionToString(D3D_SRV_DIMENSION dimension);
        bool LoadShader(EShaderType type, const std::filesystem::path& path, ID3DBlob* pBlob);
        bool MakeTextureSRV(const TextureSRVParams& params);
        StringMap<uint32_t> ParseShaderResources(uint32_t numBoundResources, ID3D11ShaderReflection* reflection);

        // Helper to cast into the correct shader resource type
        template<typename SourceType, typename DestType, typename ReturnType>
        static ReturnType* castGenericResourceToDX11Resource(const SourceType* src)
        {
            if (src == nullptr)
                return static_cast<ReturnType*>(nullptr);

            auto view = static_cast<const DestType*>(src);
            return static_cast<ReturnType*>(view->resource_.Get());
        }

    private:
        static constexpr std::string_view ShaderExt = ".cso";

        ERenderer type_;
        DX11Device device_;
        DX11Context context_;
        StringMap<DX11ComputeShader> shaders_;
        StringMap<DX11Buffer> cBuffers_;
        std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> samplers_;

        // tracks allocated resources
        ObjectTracker tracker_;

        // swap chain
        DXGISwapChain swapchain_;
    };
} // namespace ninniku
