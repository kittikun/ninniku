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

#include "../../../utils/objectTracker.h"
#include "../../../utils/stringMap.h"

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
    // DX11BufferInternal
    //////////////////////////////////////////////////////////////////////////
    struct DX11BufferInternal final : TrackedObject
    {
        DX11Buffer _buffer;
        SRVHandle _srv;
        UAVHandle _uav;

        // leave data here to support update later on
        std::vector<uint32_t> _data;

        // Initial desc that was used to create the resource
        std::shared_ptr<const BufferParam> _desc;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11BufferImpl
    //////////////////////////////////////////////////////////////////////////
    struct DX11BufferImpl : public BufferObject
    {
        DX11BufferImpl(const std::shared_ptr<DX11BufferInternal>& impl) noexcept;

        const std::vector<uint32_t>& GetData() const override;
        const BufferParam* GetDesc() const override;
        const ShaderResourceView* GetSRV() const override;
        const UnorderedAccessView* GetUAV() const override;

        std::weak_ptr<DX11BufferInternal> _impl;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11ComputeShader
    //////////////////////////////////////////////////////////////////////////
    struct DX11ComputeShader
    {
        // only allows rvalue construction (to std::pair)
        DX11ComputeShader(DX11ComputeShader&&) = default;
        DX11ComputeShader(const DX11ComputeShader&) = delete;
        DX11ComputeShader& operator=(DX11ComputeShader&) = delete;
        DX11ComputeShader& operator=(DX11ComputeShader&&) = delete;

        DX11CS shader;
        StringMap<uint32_t> bindSlots;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11DebugMarker
    //////////////////////////////////////////////////////////////////////////

    struct DX11DebugMarker final : public DebugMarker
    {
    public:
        DX11DebugMarker(const DX11Marker& marker, const std::string_view& name);
        ~DX11DebugMarker() override;

    private:
        DX11Marker _marker;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11MappedResource
    //////////////////////////////////////////////////////////////////////////
    struct DX11MappedResource final : public MappedResource
    {
    public:
        DX11MappedResource(const DX11Context& context, const TextureHandle& texObj, const uint32_t index);
        DX11MappedResource(const DX11Context& context, const BufferHandle& bufObj);
        ~DX11MappedResource() override;

        // MappedResource
        void* GetData() const override { return _mapped.pData; }

        D3D11_MAPPED_SUBRESOURCE* Get() { return &_mapped; }
        uint32_t GetRowPitch() const { return _mapped.RowPitch; }

    private:
        std::variant<const struct DX11TextureInternal*, const DX11BufferInternal*> _resource;
        const DX11Context& _context;
        const uint32_t _index;
        D3D11_MAPPED_SUBRESOURCE _mapped;
    };

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
    // DX11TextureInternal
    //////////////////////////////////////////////////////////////////////////
    struct DX11TextureInternal final : TrackedObject
    {
        ID3D11Resource* GetResource() const;

        std::variant<DX11Tex1D, DX11Tex2D, DX11Tex3D> _texture;
        SRVHandle _srvDefault;

        // D3D_SRV_DIMENSION_TEXTURECUBE
        SRVHandle _srvCube;

        // D3D_SRV_DIMENSION_TEXTURECUBEARRAY
        SRVHandle _srvCubeArray;

        // D3D_SRV_DIMENSION_TEXTURE2DARRAY per mip level
        std::vector<SRVHandle> _srvArray;

        SRVHandle _srvArrayWithMips;

        // One D3D11_TEX2D_ARRAY_UAV per mip level
        std::vector<UAVHandle> _uav;

        // Initial desc that was used to create the resource
        std::shared_ptr<const TextureParam> _desc;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11TextureImpl
    //////////////////////////////////////////////////////////////////////////
    struct DX11TextureImpl final : public TextureObject
    {
        DX11TextureImpl(const std::shared_ptr<DX11TextureInternal>& impl) noexcept;

        const TextureParam* GetDesc() const override;
        const ShaderResourceView* GetSRVDefault() const override;
        const ShaderResourceView* GetSRVCube() const override;
        const ShaderResourceView* GetSRVCubeArray() const override;
        const ShaderResourceView* GetSRVArray(uint32_t index) const override;
        const ShaderResourceView* GetSRVArrayWithMips() const override;
        const UnorderedAccessView* GetUAV(uint32_t index) const override;

        std::weak_ptr<DX11TextureInternal> _impl;
    };

    //////////////////////////////////////////////////////////////////////////
    // TextureSRVParams
    //////////////////////////////////////////////////////////////////////////
    struct TextureSRVParams : NonCopyable
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
} // namespace ninniku
