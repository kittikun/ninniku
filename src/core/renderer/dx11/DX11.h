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

#include "DX11Types.h"

struct ID3D11ShaderReflection;

namespace ninniku {
    class DX11 final : public RenderDevice
    {
    public:
        DX11(ERenderer type);

        // RenderDevice
        virtual ERenderer GetType() const override { return _type; }

        void CopyBufferResource(const CopyBufferSubresourceParam& params) override;
        std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) const override;
        BufferHandle CreateBuffer(const BufferParamHandle& params) override;
        BufferHandle CreateBuffer(const BufferHandle& src) override;
        CommandHandle CreateCommand() const override { return std::make_unique<Command>(); }
        DebugMarkerHandle CreateDebugMarker(const std::string& name) const override;
        TextureHandle CreateTexture(const TextureParamHandle& params) override;
        bool Dispatch(const CommandHandle& cmd) override;
        void Finalize() override;
        bool Initialize(const std::vector<std::string>& shaderPaths) override;
        bool LoadShader(const std::string& name, const void* pData, const size_t size) override;
        MappedResourceHandle Map(const BufferHandle& bObj) override;
        MappedResourceHandle Map(const TextureHandle& tObj, const uint32_t index) override;
        bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size) override;

        const SamplerState* GetSampler(ESamplerState sampler) const override { return _samplers[static_cast<std::underlying_type<ESamplerState>::type>(sampler)].get(); }

        ID3D11Device* GetDevice() const { return _device.Get(); }

    private:
        struct TextureSRVParams
        {
            TextureObject* obj;
            TextureParamHandle texParams;
            uint8_t is1d : 1;
            uint8_t is2d : 1;
            uint8_t is3d : 1;
            uint8_t isCube : 1;
            uint8_t isCubeArray : 1;
            uint8_t padding : 3;
        };

        bool CreateDevice(int adapter, ID3D11Device** pDevice);
        bool LoadShader(const std::string& name, ID3DBlob* pBlob, const std::string& path);
        bool LoadShaders(const std::string& shaderPath);
        bool MakeTextureSRV(const TextureSRVParams& params);
        std::unordered_map<std::string, uint32_t> ParseShaderResources(uint32_t numBoundResources, ID3D11ShaderReflection* reflection);

        // Helper to cast into the correct shader resource type
        template<typename SourceType, typename DestType, typename ReturnType>
        static ReturnType* castGenericResourceToDX11Resource(const SourceType* src)
        {
            auto view = static_cast<const DestType*>(src);
            return static_cast<ReturnType*>(view->_resource.Get());
        }

    private:
        ERenderer _type;
        DX11Device _device;
        DX11Context _context;
        std::unordered_map<std::string, DX11ComputeShader> _shaders;
        std::unordered_map<std::string, DX11Buffer> _cBuffers;
        std::array<SSHandle, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> _samplers;
    };
} // namespace ninniku
