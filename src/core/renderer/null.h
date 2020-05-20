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

namespace ninniku {
    class NullRenderer final : public RenderDevice
    {
    public:
        NullRenderer() = default;

        // RenderDevice
        ERenderer GetType() const override { return ERenderer::RENDERER_NULL; }

        const std::string_view& GetShaderExtension() const override { throw std::exception("Invalid for RENDERER_NULL"); }
        void CopyBufferResource([[maybe_unused]] const CopyBufferSubresourceParam& params) override { throw std::exception("Invalid for RENDERER_NULL"); }
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource([[maybe_unused]] const CopyTextureSubresourceParam& params) override { throw std::exception("Invalid for RENDERER_NULL"); }
        BufferHandle CreateBuffer([[maybe_unused]] const BufferParamHandle& params) override { throw std::exception("Invalid for RENDERER_NULL"); }
        BufferHandle CreateBuffer([[maybe_unused]] const BufferHandle& src) override { throw std::exception("Invalid for RENDERER_NULL"); }
        CommandHandle CreateCommand() const override { throw std::exception("Invalid for RENDERER_NULL"); }
        DebugMarkerHandle CreateDebugMarker([[maybe_unused]] const std::string_view& name) const override { throw std::exception("Invalid for RENDERER_NULL"); }
        TextureHandle CreateTexture([[maybe_unused]] const TextureParamHandle& params) override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool Dispatch([[maybe_unused]] const CommandHandle& cmd) override { throw std::exception("Invalid for RENDERER_NULL"); }
        void Finalize() override {}
        bool Initialize() override { return true; }
        bool LoadShader([[maybe_unused]] const std::filesystem::path& path) override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool LoadShader([[maybe_unused]] const std::string_view& name, [[maybe_unused]] const void* pData, [[maybe_unused]] const size_t size) override { throw std::exception("Invalid for RENDERER_NULL"); }
        MappedResourceHandle Map([[maybe_unused]] const BufferHandle& bObj) override { throw std::exception("Invalid for RENDERER_NULL"); }
        MappedResourceHandle Map([[maybe_unused]] const TextureHandle& tObj, [[maybe_unused]] const uint32_t index) override { throw std::exception("Invalid for RENDERER_NULL"); }
        bool UpdateConstantBuffer([[maybe_unused]] const std::string_view& name, [[maybe_unused]] void* data, [[maybe_unused]] const uint32_t size) override { throw std::exception("Invalid for RENDERER_NULL"); }
        const SamplerState* GetSampler([[maybe_unused]] ESamplerState sampler) const override { throw std::exception("Invalid for RENDERER_NULL"); }
    };
} // namespace ninniku
