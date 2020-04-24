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

#include "ninniku/core/renderer/types.h"

#include <wrl/client.h>
#include <d3d11_1.h>
#include <variant>
#include <vector>

namespace ninniku
{
    using DX11Buffer = Microsoft::WRL::ComPtr<ID3D11Buffer>;
    using DX11Context = Microsoft::WRL::ComPtr<ID3D11DeviceContext>;
    using DX11CS = Microsoft::WRL::ComPtr<ID3D11ComputeShader>;
    using DX11Device = Microsoft::WRL::ComPtr<ID3D11Device>;
    using DX11Marker = Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>;
    using DX11Tex1D = Microsoft::WRL::ComPtr<ID3D11Texture1D>;
    using DX11Tex2D = Microsoft::WRL::ComPtr<ID3D11Texture2D>;
    using DX11Tex3D = Microsoft::WRL::ComPtr<ID3D11Texture3D>;
    using DX11SS = Microsoft::WRL::ComPtr<ID3D11SamplerState>;
    using DX11SRV = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
    using DX11UAV = Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>;

    //////////////////////////////////////////////////////////////////////////
    // DX11 Shader Resources
    //////////////////////////////////////////////////////////////////////////
    struct DX11ShaderResourceView final : public ShaderResourceView
    {
    public:
        DX11SRV _resource;
    };

    struct DX11UnorderedAccessView final : public UnorderedAccessView
    {
    public:
        DX11UAV _resource;
    };

    struct DX11SamplerState final : public SamplerState
    {
    public:
        DX11SS _resource;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11BufferObject
    //////////////////////////////////////////////////////////////////////////
    struct DX11BufferObject final : public BufferObject
    {
    public:
        DX11BufferObject() = default;

        // BufferObject
        const std::vector<uint32_t>& GetData() const override { return _data; }
        const ShaderResourceView* GetSRV() const override { return _srv.get(); }
        const UnorderedAccessView* GetUAV() const override { return _uav.get(); }

    public:
        DX11Buffer _buffer;
        SRVHandle _srv;
        UAVHandle _uav;

        // leave data here to support update later on
        std::vector<uint32_t> _data;
    };

    static BufferHandle Empty_BufferHandleDX11;

    //////////////////////////////////////////////////////////////////////////
    // DX11DebugMarker
    //////////////////////////////////////////////////////////////////////////

    struct DX11DebugMarker final : public DebugMarker
    {
    public:
        DX11DebugMarker(const DX11Marker& marker, const std::string& name);
        ~DX11DebugMarker() override;

    private:
        DX11Marker _marker;
    };

    //////////////////////////////////////////////////////////////////////////
    // Shader
    //////////////////////////////////////////////////////////////////////////

    struct ComputeShader
    {
        // only allows rvalue construction (to std::pair)
        ComputeShader(ComputeShader&&) = default;
        ComputeShader(const ComputeShader&) = delete;
        ComputeShader& operator=(ComputeShader&) = delete;
        ComputeShader& operator=(ComputeShader&&) = delete;

        DX11CS shader;
        std::unordered_map<std::string, uint32_t> bindSlots;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureObject
    //////////////////////////////////////////////////////////////////////////
    struct DX11TextureObject final : public TextureObject
    {
    public:
        DX11TextureObject() = default;

        const ShaderResourceView* GetSRVDefault() const { return srvDefault.get(); }
        const ShaderResourceView* GetSRVCube() const { return srvCube.get(); }
        const ShaderResourceView* GetSRVCubeArray() const { return srvCubeArray.get(); }
        const ShaderResourceView* GetSRVArray(uint32_t index) const { return srvArray[index].get(); }
		const ShaderResourceView* GetSRVArrayWithMips() const { return srvArrayWithMips.get(); }
        const UnorderedAccessView* GetUAV(uint32_t index) const { return uav[index].get(); }

        ID3D11Resource* GetResource() const;

    public:
        std::variant<DX11Tex1D, DX11Tex2D, DX11Tex3D> texture;
        SRVHandle srvDefault;

        // D3D_SRV_DIMENSION_TEXTURECUBE
        SRVHandle srvCube;

        // D3D_SRV_DIMENSION_TEXTURECUBEARRAY
        SRVHandle srvCubeArray;

        // D3D_SRV_DIMENSION_TEXTURE2DARRAY per mip level
        std::vector<SRVHandle> srvArray;

        SRVHandle srvArrayWithMips;

        // One D3D11_TEX2D_ARRAY_UAV per mip level
        std::vector<UAVHandle> uav;
    };

    static TextureHandle Empty_TextureHandle;

    //////////////////////////////////////////////////////////////////////////
    // GPU to CPU readback
    //////////////////////////////////////////////////////////////////////////

    struct DX11MappedResource final : public MappedResource
    {
    public:
        DX11MappedResource(const DX11Context& context, const TextureHandle& texObj, const uint32_t index);
        DX11MappedResource(const DX11Context& context, const BufferHandle& bufObj);
        ~DX11MappedResource() override;

        // MappedResource
        void* GetData() const override { return _mapped.pData; }
        uint32_t GetRowPitch() const override;

        D3D11_MAPPED_SUBRESOURCE* Get() { return &_mapped; }

    private:
        const BufferHandle& _bufferObj;
        const DX11Context& _context;
        const TextureHandle& _texObj;
        const uint32_t _index;
        D3D11_MAPPED_SUBRESOURCE _mapped;
    };
} // namespace ninniku
