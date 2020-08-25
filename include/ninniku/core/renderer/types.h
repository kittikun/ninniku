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

#include "../../utils.h"

#include <array>
#include <DirectXMath.h>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace ninniku
{
    static constexpr uint32_t CUBEMAP_NUM_FACES = 6;

    //////////////////////////////////////////////////////////////////////////
    // Common enumerations
    //////////////////////////////////////////////////////////////////////////
    enum ECommandType : uint8_t
    {
        CT_Compute,
        CT_Graphic
    };

    enum EDeviceFeature : uint8_t
    {
        DF_ALLOW_TEARING,
        DF_SM6_WAVE_INTRINSICS,
        DF_COUNT
    };

    enum EResourceViews : uint8_t
    {
        RV_None = 0,                        // Should not be used
        RV_SRV = 1 << 0,
        RV_UAV = 1 << 1,
        RV_CPU_READ = 1 << 2
    };

    enum EShaderType : uint8_t
    {
        ST_Compute,
        ST_Pixel,
        ST_Root_Signature,
        ST_Vertex,
        ST_Count
    };

    enum class ESamplerState : uint8_t
    {
        SS_Point,
        SS_Linear,
        SS_Count
    };

    enum ETextureFormat : uint8_t
    {
        TF_UNKNOWN,
        TF_R8_UNORM,
        TF_R8G8_UNORM,
        TF_R8G8B8A8_UNORM,
        TF_R11G11B10_FLOAT,
        TF_R16_UNORM,
        TF_R16G16_UNORM,
        TF_R16G16B16A16_FLOAT,
        TF_R16G16B16A16_UNORM,
        TF_R32_FLOAT,
        TF_R32G32B32A32_FLOAT
    };

    //////////////////////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////////////////////
    struct BufferParam : NonCopyable
    {
        static NINNIKU_API std::shared_ptr<BufferParam> Create();
        NINNIKU_API std::shared_ptr<BufferParam> Duplicate() const;

        uint32_t numElements;
        uint32_t elementSize;        // If != 0, this will create a StructuredBuffer, otherwise a ByteAddressBuffer will be created
        uint8_t viewflags;
    };

    using BufferParamHandle = std::shared_ptr<const BufferParam>;

    struct BufferObject : NonCopyable
    {
        // This will only be filled when copied from another buffer (they are mapped)
        // Add support for initial data later
        virtual const std::tuple<uint8_t*, uint32_t> GetData() const = 0;

        virtual const BufferParam* GetDesc() const = 0;
        virtual const struct ShaderResourceView* GetSRV() const = 0;
        virtual const struct UnorderedAccessView* GetUAV() const = 0;
    };

    using BufferHandle = std::unique_ptr<const BufferObject>;

    //////////////////////////////////////////////////////////////////////////
    // Commands
    //////////////////////////////////////////////////////////////////////////
    struct Command : NonCopyable
    {
        Command(ECommandType type) noexcept;
        virtual ~Command() = default;

        ECommandType type_;
        std::string_view cbufferStr;
        std::unordered_map<std::string_view, const struct ShaderResourceView*> srvBindings;
        std::unordered_map<std::string_view, const struct UnorderedAccessView*> uavBindings;
        std::unordered_map<std::string_view, const struct RenderTargetView*> rtvBindings;
        std::unordered_map<std::string_view, const struct SamplerState*> ssBindings;
    };

    struct ComputeCommand : Command
    {
        ComputeCommand() noexcept;

        std::string_view shader;
        std::array<uint32_t, 3> dispatch;
    };

    using ComputeCommandHandle = std::unique_ptr<ComputeCommand>;

    struct GraphicCommand : Command
    {
        GraphicCommand() noexcept;
    };

    using GraphicCommandHandle = std::unique_ptr<GraphicCommand>;

    struct ClearRenderTargetParam
    {
        DirectX::XMVECTORF32 color;
        const RenderTargetView* dstRT;
        uint32_t index; // change this later
    };

    struct CopyBufferSubresourceParam : NonCopyable
    {
        const BufferObject* src;
        const BufferObject* dst;
    };

    struct CopyTextureSubresourceParam : NonCopyable
    {
        const struct TextureObject* src;
        uint32_t srcFace;
        uint32_t srcMip;
        const struct TextureObject* dst;
        uint32_t dstFace;
        uint32_t dstMip;
    };

    struct SubresourceParam
    {
        void* data;

        // No need for 1D textures
        uint32_t rowPitch;

        // only for 3D textures
        uint32_t depthPitch;
    };

    //////////////////////////////////////////////////////////////////////////
    // Debug
    //////////////////////////////////////////////////////////////////////////

    struct NINNIKU_API DebugMarker : NonCopyable
    {
    };

    using DebugMarkerHandle = std::unique_ptr<const DebugMarker>;

    //////////////////////////////////////////////////////////////////////////
    // MappedResource: GPU to CPU read back
    //////////////////////////////////////////////////////////////////////////

    struct MappedResource : NonCopyable
    {
        virtual void* GetData() const = 0;
    };

    using MappedResourceHandle = std::unique_ptr<const MappedResource>;

    //////////////////////////////////////////////////////////////////////////
    // Pipeline State
    //////////////////////////////////////////////////////////////////////////
    struct PipelineStateParam : NonCopyable
    {
        std::array<std::string, EShaderType::ST_Count> shaders_;
    };

    //////////////////////////////////////////////////////////////////////////
    // Shader Resources
    //////////////////////////////////////////////////////////////////////////
    struct ShaderResourceView : NonCopyable
    {
    };

    using SRVHandle = std::unique_ptr<ShaderResourceView>;

    struct UnorderedAccessView : NonCopyable
    {
    };

    using UAVHandle = std::unique_ptr<UnorderedAccessView>;

    struct RenderTargetView : NonCopyable
    {
    };

    using RTVHandle = std::unique_ptr<RenderTargetView>;

    struct SamplerState : NonCopyable
    {
    };

    using SSHandle = std::unique_ptr<SamplerState>;

    //////////////////////////////////////////////////////////////////////////
    // Swap chain
    //////////////////////////////////////////////////////////////////////////
    struct SwapchainParam : NonCopyable
    {
        static NINNIKU_API std::shared_ptr<SwapchainParam> Create();

        uint8_t format;
        uint8_t bufferCount;
        uint32_t height;
        HWND hwnd;
        bool vsync;
        uint32_t width;
    };

    using SwapchainParamHandle = std::shared_ptr<const SwapchainParam>;

    struct SwapChainObject : NonCopyable
    {
        virtual uint32_t GetCurrentBackBufferIndex() const = 0;
        virtual const SwapchainParam* GetDesc() const = 0;
        virtual const RenderTargetView* GetRT(uint32_t index) const = 0;
        virtual uint32_t GetRTCount() const = 0;
    };

    using SwapChainHandle = std::unique_ptr<const SwapChainObject>;

    //////////////////////////////////////////////////////////////////////////
    // Textures
    //////////////////////////////////////////////////////////////////////////
    struct TextureParam : NonCopyable
    {
        static NINNIKU_API std::shared_ptr<TextureParam> Create();
        NINNIKU_API std::shared_ptr<TextureParam> Duplicate() const;

        uint32_t numMips;
        uint32_t arraySize;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint8_t viewflags;
        uint8_t format;

        // one per face/mip/array etc..
        std::vector<SubresourceParam> imageDatas;
    };

    using TextureParamHandle = std::shared_ptr<const TextureParam>;

    struct TextureObject : NonCopyable
    {
        virtual const TextureParam* GetDesc() const = 0;
        virtual const ShaderResourceView* GetSRVDefault() const = 0;
        virtual const ShaderResourceView* GetSRVCube() const = 0;
        virtual const ShaderResourceView* GetSRVCubeArray() const = 0;
        virtual const ShaderResourceView* GetSRVArray(uint32_t index) const = 0;
        virtual const ShaderResourceView* GetSRVArrayWithMips() const = 0;
        virtual const UnorderedAccessView* GetUAV(uint32_t index) const = 0;
    };

    using TextureHandle = std::unique_ptr<const TextureObject>;
} // namespace ninniku