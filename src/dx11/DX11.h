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

#include "DX11Types.h"

#include <d3d11shader.h>

namespace ninniku {
class DX11
{
public:
    class DebugMarker
    {
    public:
        DebugMarker(const DX11Marker& marker, const std::string& name);
        ~DebugMarker();

    private:
        DX11Marker _marker;
    };

    class MappedResource
    {
    public:
        MappedResource(const DX11Context& context, const std::unique_ptr<TextureObject>& texObj, uint32_t index);
        ~MappedResource();

        D3D11_MAPPED_SUBRESOURCE* Get()
        {
            return &_mapped;
        }
        void* GetData() const
        {
            return _mapped.pData;
        }
        uint32_t GetRowPitch() const;

    private:
        const DX11Context& _context;
        const std::unique_ptr<TextureObject>& _texObj;
        uint32_t _index;
        D3D11_MAPPED_SUBRESOURCE _mapped;
    };

    std::tuple<uint32_t, uint32_t> CopySubresource(const CopySubresourceParam& params) const;
    std::unique_ptr<DebugMarker> CreateDebugMarker(const std::string& name) const;
    std::unique_ptr<TextureObject> CreateTexture(const TextureParam& param);
    bool Dispatch(const Command& cmd) const;
    bool Initialize();
    std::unique_ptr<MappedResource> MapTexture(const std::unique_ptr<TextureObject>& tObj, uint32_t index);
    bool UpdateConstantBuffer(const std::string& name, void* data, uint32_t size);

    const DX11SamplerState& GetSampler(ESamplerState sampler) const { return _samplers[sampler]; }

private:
    bool CreateDevice(int adapter, ID3D11Device** pDevice);
    bool GetDXGIFactory(IDXGIFactory1** pFactory);
    bool LoadShaders();
    std::unordered_map<std::string, uint32_t> ParseShaderResources(const D3D11_SHADER_DESC& desc, ID3D11ShaderReflection* reflection);

private:
    DX11Device _device;
    DX11Context _context;
    std::unordered_map<std::string, ComputeShader> _shaders;
    std::unordered_map<std::string, DX11Buffer> _cBuffers;
    std::array<DX11SamplerState, SS_Count> _samplers;
};
} // namespace ninniku
