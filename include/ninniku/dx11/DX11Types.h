#pragma once

#include "../types.h"

#include <wrl/client.h>
#include <d3d11_1.h>
#include <unordered_map>

namespace ninniku
{
    using DX11Marker = Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>;
    using DX11Buffer = Microsoft::WRL::ComPtr<ID3D11Buffer>;
    using DX11Context = Microsoft::WRL::ComPtr<ID3D11DeviceContext>;
    using DX11CS = Microsoft::WRL::ComPtr<ID3D11ComputeShader>;
    using DX11Device = Microsoft::WRL::ComPtr<ID3D11Device>;
    using DX11Tex2D = Microsoft::WRL::ComPtr<ID3D11Texture2D>;
    using DX11SamplerState = Microsoft::WRL::ComPtr<ID3D11SamplerState>;
    using DX11SRV = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
    using DX11UAV = Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>;

    struct Command
    {
        // no copy of any kind allowed
        Command(const Command&) = delete;
        Command& operator=(Command&) = delete;
        Command(Command&&) = delete;
        Command& operator=(Command&&) = delete;

        std::string shader;
        std::string cbufferStr;
        std::unordered_map<std::string, DX11SRV> srvBindings;
        std::unordered_map<std::string, DX11UAV> uavBindings;
        std::unordered_map<std::string, DX11SamplerState> ssBindings;
        std::array<uint32_t, 3> dispatch;
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
    // Textures
    //////////////////////////////////////////////////////////////////////////

    struct TextureObject
    {
        // no copy of any kind allowed
        TextureObject(const TextureObject&) = delete;
        TextureObject& operator=(TextureObject&) = delete;
        TextureObject(TextureObject&&) = delete;
        TextureObject& operator=(TextureObject&&) = delete;

        TextureObject() = default;

        DX11Tex2D texture;

        DX11SRV srvDefault;

        // D3D_SRV_DIMENSION_TEXTURECUBE
        DX11SRV srvCube;

        // D3D_SRV_DIMENSION_TEXTURE2DARRAY per mip level
        std::vector<DX11SRV> srvArray;

        DX11SRV srvArrayWithMips;

        // One D3D11_TEX2D_ARRAY_UAV per mip level
        std::vector<DX11UAV> uav;

        // Desc that was used to create those resources
        TextureParam desc;
    };
} // namespace ninniku