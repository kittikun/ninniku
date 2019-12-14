// Copyright(c) 2018-2019 Kitti Vongsay
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

#include "DX12Types.h"

namespace ninniku
{
    class DX12 final : public RenderDevice
    {
    public:
        ERenderer GetType() const override { return ERenderer::RENDERER_DX12; }

        void CopyBufferResource(const CopyBufferSubresourceParam& params) const override;
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) const override;
        BufferHandle CreateBuffer(const BufferParamHandle& params) override;
        BufferHandle CreateBuffer(const BufferHandle& src) override;
        CommandHandle CreateCommand() const override;
        DebugMarkerHandle CreateDebugMarker(const std::string& name) const override;
        TextureHandle CreateTexture(const TextureParamHandle& params) override;
        bool Dispatch(const CommandHandle& cmd) const override;
        bool Initialize(const std::vector<std::string>& shaderPaths, const bool isWarp) override;
        bool LoadShader(const std::string& name, const void* pData, const size_t size) override;
        MappedResourceHandle MapBuffer(const BufferHandle& bObj) override;
        MappedResourceHandle MapTexture(const TextureHandle& tObj, const uint32_t index) override;
        bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size) override;

        const SamplerState* GetSampler(ESamplerState sampler) const override { return _samplers[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

    private:
        bool CreateDevice(int adapter);

    private:
        DX12Device _device;
        std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> _samplers;
    };
} // namespace ninniku
