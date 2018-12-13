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

#include "pch.h"
#include "process.h"

#include "shaders/cbuffers.h"
#include "utils/log.h"
#include "utils/mathUtils.h"
#include "dx11/DX11.h"
#include "image/cmft.h"
#include "image/dds.h"

#include <renderdoc_app.h>

#include <string>

namespace {
    uint32_t numSamples = 10000;
    uint32_t numStepSamples = std::min(2000u, numSamples);
}

namespace ninniku {
    Processor::Processor(const std::shared_ptr<DX11>& dx)
        : _dx{ dx }
    {
    }

    /// <summary>
    /// Generate a texture will a different color per mip level
    /// </summary>
    void Processor::ColorMips()
    {
        auto marker = _dx->CreateDebugMarker("ColorMips");
        LOG_INDENT_START << "Running ColorMips..";

        TextureParam param = {};
        param.width = param.height = 512;
        param.numMips = CountMips(std::min(param.width, param.height));
        param.arraySize = CUBEMAP_NUM_FACES;
        param.viewflags = TV_SRV | TV_UAV;

        auto resTex = _dx->CreateTexture(param);

        if (resTex) {
            for (uint32_t i = 0; i < param.numMips; ++i) {
                // dispatch
                Command cmd = {};
                cmd.shader = "colorMips";
                cmd.cbufferStr = "CBGlobal";

                static_assert((COLORMIPS_NUMTHREAD_X == COLORMIPS_NUMTHREAD_Y) && (COLORMIPS_NUMTHREAD_Z == 1));
                cmd.dispatch[0] = std::max(1u, (param.width >> i) / COLORMIPS_NUMTHREAD_X);
                cmd.dispatch[1] = std::max(1u, (param.height >> i) / COLORMIPS_NUMTHREAD_Y);
                cmd.dispatch[2] = CUBEMAP_NUM_FACES / COLORMIPS_NUMTHREAD_Z;

                cmd.uavBindings.insert(std::make_pair("dstTex", resTex->uav[i]));

                // constant buffer
                CBGlobal cb = {};

                cb.targetMip = i;
                _dx->UpdateConstantBuffer(cmd.cbufferStr, &cb, sizeof(CBGlobal));

                _dx->Dispatch(cmd);
            }

            // save output to dds
            auto tmp = ImageFromTextureObject(resTex);
            //tmp->SaveImage("colorMips");
            tmp->SaveImageFaceList("colorFace_original");
        }

        LOG_INDENT_END;
    }

    /// <summary>
    /// Generate mips via compute shader
    /// 1: Create a new cubemap with mips and copy srcTex mip 0 to it
    /// 2: Generate remaing mips by box sampling the upper mip
    /// </summary>
    void Processor::GenerateMips(const std::unique_ptr<TextureObject>& srcTex)
    {
        auto marker = _dx->CreateDebugMarker("Downsample");
        LOG_INDENT_START << "Running GenerateMips..";

        if (srcTex->desc.numMips > 1) {
            LOGW << "Source texture already contains mips, they will be ignored and recreated from mip 0";
        }

        TextureParam param = {};

        param.width = srcTex->desc.width;
        param.height = srcTex->desc.height;
        param.numMips = CountMips(std::min(param.width, param.height));
        param.arraySize = CUBEMAP_NUM_FACES;
        param.viewflags = TV_SRV | TV_UAV;

        auto resTex = _dx->CreateTexture(param);

        if (resTex) {
            // copy srcTex mip 0 to resTex
            {
                auto subMarker = _dx->CreateDebugMarker("Copy mip 0");
                CopySubresourceParam copyParams = {};

                copyParams.src = srcTex.get();
                copyParams.dst = resTex.get();

                for (uint16_t i = 0; i < CUBEMAP_NUM_FACES; ++i) {
                    copyParams.srcFace = i;
                    copyParams.dstFace = i;

                    _dx->CopySubresource(copyParams);
                }
            }

            // mip generation
            {
                auto subMarker = _dx->CreateDebugMarker("Generate remaining mips");

                for (uint32_t srcMip = 0; srcMip < param.numMips - 1; ++srcMip) {
                    // dispatch
                    Command cmd = {};
                    cmd.shader = "downsample";
                    cmd.cbufferStr = "CBGlobal";

                    static_assert((DOWNSAMPLE_NUMTHREAD_X == DOWNSAMPLE_NUMTHREAD_Y) && (DOWNSAMPLE_NUMTHREAD_Z == 1));
                    cmd.dispatch[0] = std::max(1u, (param.width >> srcMip) / DOWNSAMPLE_NUMTHREAD_X);
                    cmd.dispatch[1] = std::max(1u, (param.height >> srcMip) / DOWNSAMPLE_NUMTHREAD_Y);
                    cmd.dispatch[2] = CUBEMAP_NUM_FACES / DOWNSAMPLE_NUMTHREAD_Z;

                    cmd.srvBindings.insert(std::make_pair("srcMip", resTex->srvArray[srcMip]));
                    cmd.uavBindings.insert(std::make_pair("dstMipSlice", resTex->uav[srcMip + 1]));
                    cmd.ssBindings.insert(std::make_pair("ssPoint", _dx->GetSampler(SS_Point)));

                    // constant buffer
                    CBGlobal cb = {};

                    cb.targetMip = srcMip;
                    _dx->UpdateConstantBuffer(cmd.cbufferStr, &cb, sizeof(CBGlobal));

                    _dx->Dispatch(cmd);
                }
            }

            // save output to dds
            {
                auto tmp = ImageFromTextureObject(resTex);
                tmp->SaveImage("downsample");
            }
        }

        LOG_INDENT_END;
    }

    std::unique_ptr<cmftImage> Processor::ImageFromTextureObject(const std::unique_ptr<TextureObject>& srcTex)
    {
        auto marker = _dx->CreateDebugMarker("ImageFromTextureObject");
        LOG_INDENT_START << "Making Image from TextureObject..";

        auto res = std::make_unique<cmftImage>(srcTex->desc.width, srcTex->desc.height, srcTex->desc.numMips);

        // we have to copy each mip with a read back texture or the same size for each face
        for (uint32_t mip = 0; mip < srcTex->desc.numMips; ++mip) {
            TextureParam param = {};

            param.width = srcTex->desc.width >> mip;
            param.height = srcTex->desc.height >> mip;
            param.format = srcTex->desc.format;
            param.numMips = 1;
            param.arraySize = 1;
            param.viewflags = TV_CPU_READ;

            auto readBack = _dx->CreateTexture(param);

            CopySubresourceParam params = {};
            params.src = srcTex.get();
            params.srcMip = mip;
            params.dst = readBack.get();

            for (uint32_t face = 0; face < CUBEMAP_NUM_FACES; ++face) {
                params.srcFace = face;

                auto indexes = _dx->CopySubresource(params);
                auto mapped = _dx->MapTexture(readBack, std::get<1>(indexes));

                res->UpdateSubImage(face, mip, (uint8_t*)mapped->GetData(), mapped->GetRowPitch());
            }
        }

        LOG_INDENT_END;

        return res;
    }

    bool Processor::ProcessImage(const boost::filesystem::path& path)
    {
        auto image = std::make_unique<cmftImage>();

        if (!image->Load(path.string())) {
            LOGE << "Failed to create cmftImage";
            return false;
        }

        auto param = image->CreateTextureParam(TV_SRV);
        auto original = _dx->CreateTexture(param);

        // check if we need to resize
        auto fix = image->IsRequiringFix();

        if (std::get<0>(fix)) {
            original = ResizeImage(std::get<1>(fix), original, image);
            image->SaveImage(path.stem().string());
        }

        ColorMips();
        GenerateMips(original);
        TestCubemapDirToTexture2DArray(original);
        TestDDS();

        return true;
    }

    std::unique_ptr<TextureObject> Processor::ResizeImage(uint32_t newSize, const std::unique_ptr<TextureObject>& srcTex, std::unique_ptr<cmftImage>& srcImg)
    {
        auto marker = _dx->CreateDebugMarker("Resize");
        auto fmt = boost::format("Resizing to %1%x%1%..") % newSize;
        LOG_INDENT_START << boost::str(fmt);

        TextureParam param = {};
        param.width = newSize;
        param.height = newSize;
        param.format = srcTex->desc.format;
        param.numMips = 1;
        param.arraySize = CUBEMAP_NUM_FACES;
        param.viewflags = TV_SRV | TV_UAV;

        auto dst = _dx->CreateTexture(param);

        // dispatch
        Command cmd = {};
        cmd.shader = "resize";
        cmd.ssBindings.insert(std::make_pair("ssLinear", _dx->GetSampler(SS_Linear)));
        cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvArray[0]));
        cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[0]));

        static_assert((RESIZE_NUMTHREAD_X == RESIZE_NUMTHREAD_Y) && (RESIZE_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = cmd.dispatch[1] = newSize / RESIZE_NUMTHREAD_X;
        cmd.dispatch[2] = CUBEMAP_NUM_FACES / RESIZE_NUMTHREAD_Z;

        _dx->Dispatch(cmd);

        // overwrite srcImg too
        srcImg = ImageFromTextureObject(dst);

        LOG_INDENT_END;

        return dst;
    }

    /// <summary>
    /// Test if CubemapDirToTexture2DArray is working as expect
    /// 1: Create a new cube map with no mips and color each face
    /// 2: Create another cube map and bind the first as cube and sample it Texture2DArray style
    /// </summary>
    void Processor::TestCubemapDirToTexture2DArray(const std::unique_ptr<TextureObject>& original)
    {
        auto marker = _dx->CreateDebugMarker("TestCubemapDirToTexture2DArray");
        LOG_INDENT_START << "Running TestCubemapDirToTexture2DArray..";

        TextureParam param = {};
        param.width = param.height = 512;
        param.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        param.numMips = 1;
        param.arraySize = CUBEMAP_NUM_FACES;
        param.viewflags = TV_SRV | TV_UAV;

        auto srcTex = _dx->CreateTexture(param);
        auto dstTex = _dx->CreateTexture(param);

        // generate source texture
        {
            auto subMarker = _dx->CreateDebugMarker("Source Texture");

            // dispatch
            Command cmd = {};
            cmd.shader = "colorFaces";

            static_assert((COLORFACES_NUMTHREAD_X == COLORFACES_NUMTHREAD_Y) && (COLORFACES_NUMTHREAD_Z == 1));
            cmd.dispatch[0] = param.width / COLORFACES_NUMTHREAD_X;
            cmd.dispatch[1] = param.height / COLORFACES_NUMTHREAD_Y;
            cmd.dispatch[2] = CUBEMAP_NUM_FACES / COLORFACES_NUMTHREAD_Z;

            cmd.uavBindings.insert(std::make_pair("dstTex", srcTex->uav[0]));

            _dx->Dispatch(cmd);
        }

        // generate destination texture by sampling source using direction vectors
        {
            auto subMarker = _dx->CreateDebugMarker("Destination Texture");

            // dispatch
            Command cmd = {};
            cmd.shader = "dirToFaces";

            static_assert((DIRTOFACE_NUMTHREAD_X == DIRTOFACE_NUMTHREAD_Y) && (DIRTOFACE_NUMTHREAD_Z == 1));
            cmd.dispatch[0] = param.width / DIRTOFACE_NUMTHREAD_X;
            cmd.dispatch[1] = param.height / DIRTOFACE_NUMTHREAD_Y;
            cmd.dispatch[2] = CUBEMAP_NUM_FACES / DIRTOFACE_NUMTHREAD_Z;

            cmd.ssBindings.insert(std::make_pair("ssPoint", _dx->GetSampler(SS_Point)));
            cmd.srvBindings.insert(std::make_pair("srcTex", original->srvCube));
            cmd.uavBindings.insert(std::make_pair("dstTex", dstTex->uav[0]));

            _dx->Dispatch(cmd);
        }

        // save it on disk for reference
        {
            auto tmp = ImageFromTextureObject(srcTex);
            tmp->SaveImage("colorFace_original");
            tmp = ImageFromTextureObject(dstTex);
            tmp->SaveImage("colorFace_copy");
        }

        LOG_INDENT_END;
    }

    void Processor::TestDDS()
    {
        auto image = std::make_unique<ddsImage>();

        if (!image->Load("sampleTexture.dds")) {
            LOGE << "Failed to create ddsImage";
            return;
        }

        auto params = image->CreateTextureParam(TV_SRV);
        auto original = _dx->CreateTexture(params);

        int i = 0;
    }
} // namespace ninniku