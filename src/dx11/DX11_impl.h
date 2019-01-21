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

#include "ninniku/dx11/DX11Types.h"

#include <d3d11shader.h>

namespace ninniku
{
    class DX11Impl
    {
        // no copy of any kind allowed
        DX11Impl(const DX11Impl&) = delete;
        DX11Impl& operator=(DX11Impl&) = delete;
        DX11Impl(DX11Impl&&) = delete;
        DX11Impl& operator=(DX11Impl&&) = delete;

    public:
        DX11Impl() = default;

        std::tuple<uint32_t, uint32_t> CopySubresource(const CopySubresourceParam& params) const;
        DebugMarkerHandle CreateDebugMarker(const std::string& name) const;
        TextureHandle CreateTexture(const TextureParamHandle& param);
        bool Dispatch(const Command& cmd) const;
        bool Initialize(const std::string& shaderPath, const bool isWarp);
        MappedResourceHandle MapTexture(const TextureHandle& tObj, const uint32_t index);
        bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size);

        const DX11SamplerState& GetSampler(ESamplerState sampler) const { return _samplers[static_cast<std::underlying_type<ESamplerState>::type>(sampler)]; }

    private:
        bool CreateDevice(int adapter, ID3D11Device** pDevice);
        bool GetDXGIFactory(IDXGIFactory1** pFactory);
        bool LoadShaders(const std::string& shaderPath);
        std::unordered_map<std::string, uint32_t> ParseShaderResources(const D3D11_SHADER_DESC& desc, ID3D11ShaderReflection* reflection);

    private:
        DX11Device _device;
        DX11Context _context;
        std::unordered_map<std::string, ComputeShader> _shaders;
        std::unordered_map<std::string, DX11Buffer> _cBuffers;
        std::array<DX11SamplerState, static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Count)> _samplers;

        friend class ddsImageImpl;
    };
} // namespace ninniku