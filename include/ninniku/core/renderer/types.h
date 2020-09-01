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
    enum EBufferFlags : uint8_t
    {
        BF_NONE,
        BF_STRUCTURED_BUFFER,
        BF_VERTEX_BUFFER
    };

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

    enum EPrimitiveTopology : uint8_t
    {
        PT_TRIANGLE_LIST
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
        ST_Root_Signature,
        ST_Pixel,
        ST_Vertex,
        ST_Count
    };

    enum class ESamplerState : uint8_t
    {
        SS_Point,
        SS_Linear,
        SS_Count
    };

    enum EFormat : uint8_t
    {
        F_UNKNOWN,
        F_R8_UNORM,
        F_R8G8_UNORM,
        F_R8G8B8A8_UNORM,
        F_R11G11B10_FLOAT,
        F_R16_UNORM,
        F_R16G16_UNORM,
        F_R16G16B16A16_FLOAT,
        F_R16G16B16A16_UNORM,
        F_R32_FLOAT,
        F_R32G32B32_FLOAT,
        F_R32G32B32A32_FLOAT
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
        uint8_t bufferFlags;

        void* initData;
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
        virtual const struct VertexBufferView* GetVBV() const = 0;
    };

    using BufferHandle = std::unique_ptr<const BufferObject>;

    //////////////////////////////////////////////////////////////////////////
    // Commands
    //////////////////////////////////////////////////////////////////////////
    class Command : NonCopyable
    {
    protected:
        Command(ECommandType type) noexcept;
        virtual ~Command() = default;

    public:
        ECommandType type;
        std::string_view cbufferStr;
        std::unordered_map<std::string_view, const struct ShaderResourceView*> srvBindings;
        std::unordered_map<std::string_view, const struct UnorderedAccessView*> uavBindings;
        std::unordered_map<std::string_view, const struct RenderTargetView*> rtvBindings;
        std::unordered_map<std::string_view, const struct SamplerState*> ssBindings;
    };

    class ComputeCommand : public Command
    {
    public:
        ComputeCommand() noexcept;

        std::string_view shader;
        std::array<uint32_t, 3> dispatch;
    };

    using ComputeCommandHandle = std::unique_ptr<ComputeCommand>;

    struct ClearRenderTargetParam
    {
        DirectX::XMVECTORF32 color;
        const struct RenderTargetView* dstRT;
        uint32_t index; // change this later
    };

    struct SetVertexBuffersParam
    {
        std::vector<const VertexBufferView*> views;
    };

    class GraphicCommand : public Command
    {
    public:
        GraphicCommand() noexcept;

        virtual bool ClearRenderTarget(const ClearRenderTargetParam& params) const = 0;
        virtual bool IASetPrimitiveTopology(EPrimitiveTopology topology) const = 0;
        virtual bool IASetVertexBuffers(const SetVertexBuffersParam& params) const = 0;

        std::string_view pipelineStateName;
    };

    using GraphicCommandHandle = std::unique_ptr<GraphicCommand>;

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
    // Input Layout
    //////////////////////////////////////////////////////////////////////////

    struct InputElementDesc
    {
        std::string_view name;
        uint8_t format;
    };

    struct InputLayoutDesc : NonCopyable
    {
        std::string_view name;
        std::vector<InputElementDesc> elements;
    };

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

    class PipelineStateParam : NonCopyable
    {
    protected:
        PipelineStateParam(ECommandType type) noexcept;
        virtual ~PipelineStateParam() = default;

    public:
        ECommandType type;
        std::string_view name;
    };

    class NINNIKU_API ComputePipelineStateParam : public PipelineStateParam
    {
    public:
        ComputePipelineStateParam() noexcept;
        std::array<std::string, 2> shaders;
    };

    class NINNIKU_API GraphicPipelineStateParam : public PipelineStateParam
    {
    public:
        GraphicPipelineStateParam() noexcept;

        std::string_view inputLayout;
        uint8_t rtFormat;
        std::array<std::string, EShaderType::ST_Count> shaders;
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

    struct VertexBufferView : NonCopyable
    {
    };

    using VBVHandle = std::unique_ptr<VertexBufferView>;

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