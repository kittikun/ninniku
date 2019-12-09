#pragma once

#include "ninniku/core/renderer/types.h"

#include <wrl/client.h>
#include <d3d11_1.h>
#include <array>
#include <unordered_map>
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
    using DX11SamplerState = Microsoft::WRL::ComPtr<ID3D11SamplerState>;
    using DX11SRV = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
    using DX11UAV = Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>;

    //////////////////////////////////////////////////////////////////////////
    // DX11BufferObject
    //////////////////////////////////////////////////////////////////////////
    class DX11BufferObject : public BufferObject
    {
        // no copy of any kind allowed
        DX11BufferObject(const DX11BufferObject&) = delete;
        DX11BufferObject& operator=(DX11BufferObject&) = delete;
        DX11BufferObject(DX11BufferObject&&) = delete;
        DX11BufferObject& operator=(DX11BufferObject&&) = delete;

    public:
        DX11BufferObject() = default;

    public:
        DX11Buffer buffer;
        DX11SRV srv;
        DX11UAV uav;
    };

    static BufferHandle Empty_BufferHandle;

    //////////////////////////////////////////////////////////////////////////
    // Commands
    //////////////////////////////////////////////////////////////////////////

    struct DX11Command : Command
    {
        // no copy of any kind allowed
        DX11Command(const DX11Command&) = delete;
        DX11Command& operator=(DX11Command&) = delete;
        DX11Command(DX11Command&&) = delete;
        DX11Command& operator=(DX11Command&&) = delete;

        std::string shader;
        std::string cbufferStr;
        std::unordered_map<std::string, DX11SRV> srvBindings;
        std::unordered_map<std::string, DX11UAV> uavBindings;
        std::unordered_map<std::string, DX11SamplerState> ssBindings;
        std::array<uint32_t, 3> dispatch;
    };

    //////////////////////////////////////////////////////////////////////////
    // DX11DebugMarker
    //////////////////////////////////////////////////////////////////////////

    class DX11DebugMarker : public DebugMarker
    {
        // no copy of any kind allowed
        DX11DebugMarker(const DX11DebugMarker&) = delete;
        DX11DebugMarker& operator=(DX11DebugMarker&) = delete;
        DX11DebugMarker(DX11DebugMarker&&) = delete;
        DX11DebugMarker& operator=(DX11DebugMarker&&) = delete;

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
    class DX11TextureObject : public TextureObject
    {
        // no copy of any kind allowed
        DX11TextureObject(const DX11TextureObject&) = delete;
        DX11TextureObject& operator=(DX11TextureObject&) = delete;
        DX11TextureObject(DX11TextureObject&&) = delete;
        DX11TextureObject& operator=(DX11TextureObject&&) = delete;

    public:
        DX11TextureObject() = default;

        ID3D11Resource* GetResource() const;

    public:
        std::variant<DX11Tex1D, DX11Tex2D, DX11Tex3D> texture;
        DX11SRV srvDefault;

        // D3D_SRV_DIMENSION_TEXTURECUBE
        DX11SRV srvCube;

        // D3D_SRV_DIMENSION_TEXTURECUBEARRAY
        DX11SRV srvCubeArray;

        // D3D_SRV_DIMENSION_TEXTURE2DARRAY per mip level
        std::vector<DX11SRV> srvArray;

        DX11SRV srvArrayWithMips;

        // One D3D11_TEX2D_ARRAY_UAV per mip level
        std::vector<DX11UAV> uav;
    };

    static TextureHandle Empty_TextureHandle;

    //////////////////////////////////////////////////////////////////////////
    // GPU to CPU readback
    //////////////////////////////////////////////////////////////////////////

    class DX11MappedResource : public MappedResource
    {
        // no copy of any kind allowed
        DX11MappedResource(const DX11MappedResource&) = delete;
        DX11MappedResource& operator=(DX11MappedResource&) = delete;
        DX11MappedResource(DX11MappedResource&&) = delete;
        DX11MappedResource& operator=(DX11MappedResource&&) = delete;

    public:
        DX11MappedResource(const DX11Context& context, const TextureHandle& texObj, const uint32_t index);
        DX11MappedResource(const DX11Context& context, const BufferHandle& bufObj);
        ~DX11MappedResource();

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
