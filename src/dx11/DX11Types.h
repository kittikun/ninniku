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

static constexpr uint32_t CUBEMAP_NUM_FACES = 6;

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
    std::string shader;
    std::string cbufferStr;
    std::unordered_map<std::string, DX11SRV> srvBindings;
    std::unordered_map<std::string, DX11UAV> uavBindings;
    std::unordered_map<std::string, DX11SamplerState> ssBindings;
    std::array<uint32_t, 3> dispatch;
};

//////////////////////////////////////////////////////////////////////////
// Textures
//////////////////////////////////////////////////////////////////////////
enum ETextureViews : uint8_t
{
    TV_None = 0,
    TV_SRV = 1 << 0,
    TV_UAV = 1 << 1,
    TV_CPU_READ = 1 << 2
};

struct TextureParam
{
    uint32_t numMips;
    uint32_t arraySize;
    uint32_t size;
    uint8_t viewflags;

    // only when creating cubemaps from Image
    void* data;

    // only when creating cubemaps from Image
    uint32_t pitch;

    // only when creating cubemaps from Image
    std::array<uint32_t, CUBEMAP_NUM_FACES> faceOffsets;
};

struct TextureObject
{
    DX11Tex2D texture;

    // D3D_SRV_DIMENSION_TEXTURECUBE
    DX11SRV srvCube;

    // D3D_SRV_DIMENSION_TEXTURE2DARRAY per mip level
    std::vector<DX11SRV> srvArray;

    // One D3D11_TEX2D_ARRAY_UAV per mip level
    std::vector<DX11UAV> uav;

    // Desc that was used to create those resources
    TextureParam desc;
};

//////////////////////////////////////////////////////////////////////////
// Resources
//////////////////////////////////////////////////////////////////////////
struct CopySubresourceParam
{
    const TextureObject* src;
    uint32_t srcFace;
    uint32_t srcMip;
    const TextureObject* dst;
    uint32_t dstFace;
    uint32_t dstMip;
};

//////////////////////////////////////////////////////////////////////////
// Shader
//////////////////////////////////////////////////////////////////////////
enum ESamplerState : uint8_t
{
    SS_Point,
    SS_Linear,
    SS_Count
};

struct ComputeShader
{
    DX11CS shader;
    std::unordered_map<std::string, uint32_t> bindSlots;
};
