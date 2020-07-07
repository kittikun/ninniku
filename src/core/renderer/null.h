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

namespace ninniku
{
    class NullRenderer final : public RenderDevice
    {
    public:
        NullRenderer() = default;

        // RenderDevice
        ERenderer GetType() const override { return ERenderer::RENDERER_NULL; }

        const std::string_view& GetShaderExtension() const override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool CopyBufferResource(const CopyBufferSubresourceParam&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        BufferHandle CreateBuffer(const BufferParamHandle&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        BufferHandle CreateBuffer(const BufferHandle&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        CommandHandle CreateCommand() const override { throw std::exception("Invalid for RENDERER_NULL"); }
        DebugMarkerHandle CreateDebugMarker(const std::string_view&) const override { throw std::exception("Invalid for RENDERER_NULL"); }
        TextureHandle CreateTexture(const TextureParamHandle&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool Dispatch(const CommandHandle&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        void Finalize() override {}
        void Flush() override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool Initialize() override { return true; }
        bool LoadShader(const std::filesystem::path&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool LoadShader(const std::string_view&, const void*, const uint32_t) override { throw std::exception("Invalid for RENDERER_NULL"); }
        MappedResourceHandle Map(const BufferHandle&) override { throw std::exception("Invalid for RENDERER_NULL"); }
        MappedResourceHandle Map(const TextureHandle&, const uint32_t) override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool UpdateConstantBuffer(const std::string_view&, void*, const uint32_t) override { throw std::exception("Invalid for RENDERER_NULL"); }
        const SamplerState* GetSampler(ESamplerState) const override { throw std::exception("Invalid for RENDERER_NULL"); }
    };
} // namespace ninniku
