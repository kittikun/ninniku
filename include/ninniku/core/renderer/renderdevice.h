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

#include "../../ninniku.h"
#include "types.h"

#include <filesystem>
#include <string_view>

namespace ninniku
{
    enum EDeviceFeature : uint8_t
    {
        DF_ALLOW_TEARING,
        DF_SM6_WAVE_INTRINSICS,
        DF_COUNT
    };

    class RenderDevice : NonCopyableBase
    {
        // no copy of any kind allowed
        RenderDevice(const RenderDevice&) = delete;
        RenderDevice& operator=(RenderDevice&) = delete;
        RenderDevice(RenderDevice&&) = delete;
        RenderDevice& operator=(RenderDevice&&) = delete;

    public:
        virtual ~RenderDevice() = default;

        virtual ERenderer GetType() const = 0;
        virtual const std::string_view& GetShaderExtension() const = 0;

        [[nodiscard]] virtual bool CheckFeatureSupport(EDeviceFeature feature, bool& result) = 0;
        [[nodiscard]] virtual bool ClearRenderTarget(const ClearRenderTargetParam& params) = 0;
        [[nodiscard]] virtual bool CopyBufferResource(const CopyBufferSubresourceParam& params) = 0;
        virtual std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) = 0;
        virtual BufferHandle CreateBuffer(const BufferParamHandle& params) = 0;
        virtual BufferHandle CreateBuffer(const BufferHandle& src) = 0;
        virtual CommandHandle CreateCommand() const = 0;
        virtual DebugMarkerHandle CreateDebugMarker(const std::string_view& name) const = 0;
        [[nodiscard]] virtual SwapChainHandle CreateSwapChain(const SwapchainParamHandle& params) = 0;
        virtual TextureHandle CreateTexture(const TextureParamHandle& params) = 0;
        [[nodiscard]] virtual bool Dispatch(const CommandHandle& cmd) = 0;
        virtual void Finalize() = 0;
        [[nodiscard]] virtual bool Initialize() = 0;
        [[nodiscard]] virtual bool LoadShader(const std::filesystem::path& path) = 0;
        [[nodiscard]] virtual bool LoadShader(const std::string_view& name, const void* pData, const uint32_t size) = 0;
        [[nodiscard]] virtual MappedResourceHandle Map(const BufferHandle& bObj) = 0;
        [[nodiscard]] virtual MappedResourceHandle Map(const TextureHandle& tObj, const uint32_t index) = 0;
        [[nodiscard]] virtual bool Present(const SwapChainHandle& swapchain) = 0;
        [[nodiscard]] virtual bool UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size) = 0;

        virtual const SamplerState* GetSampler(ESamplerState sampler) const = 0;

    protected:
        RenderDevice() = default;
    };

    using RenderDeviceHandle = std::unique_ptr<RenderDevice>;

    NINNIKU_API RenderDeviceHandle& GetRenderer();
} // namespace ninniku