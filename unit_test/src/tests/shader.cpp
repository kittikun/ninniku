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

#include "../shaders/cbuffers.h"
#include "../check.h"

#include <boost/test/unit_test.hpp>
#include <ninniku/dx11/DX11.h>
#include <ninniku/image/cmft.h>
#include <ninniku/image/dds.h>
#include <ninniku/ninniku.h>
#include <ninniku/types.h>
#include <ninniku/utils.h>

BOOST_AUTO_TEST_SUITE(Shader)

BOOST_AUTO_TEST_CASE(shader_colorMips)
{
    ninniku::TextureParam param = {};
    param.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    param.width = param.height = 512;
    param.numMips = ninniku::CountMips(std::min(param.width, param.height));
    param.arraySize = ninniku::CUBEMAP_NUM_FACES;
    param.viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("ColorMips");
    auto resTex = dx->CreateTexture(param);

    for (uint32_t i = 0; i < param.numMips; ++i) {
        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "colorMips";
        cmd.cbufferStr = "CBGlobal";

        static_assert((COLORMIPS_NUMTHREAD_X == COLORMIPS_NUMTHREAD_Y) && (COLORMIPS_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = std::max(1u, (param.width >> i) / COLORMIPS_NUMTHREAD_X);
        cmd.dispatch[1] = std::max(1u, (param.height >> i) / COLORMIPS_NUMTHREAD_Y);
        cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / COLORMIPS_NUMTHREAD_Z;

        cmd.uavBindings.insert(std::make_pair("dstTex", resTex->uav[i]));

        // constant buffer
        CBGlobal cb = {};

        cb.targetMip = i;
        dx->UpdateConstantBuffer(cmd.cbufferStr, &cb, sizeof(CBGlobal));

        dx->Dispatch(cmd);
    }

    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, resTex);

    auto data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x91086088d369be49, 0x74d54476510012cc);
}

BOOST_AUTO_TEST_CASE(shader_cubemapDirToArray)
{
    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("CubemapDirToArray");

    ninniku::TextureParam param = {};
    param.width = param.height = 512;
    param.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    param.numMips = 1;
    param.arraySize = ninniku::CUBEMAP_NUM_FACES;
    param.viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto srcTex = dx->CreateTexture(param);
    auto dstTex = dx->CreateTexture(param);

    // generate source texture
    {
        auto subMarker = dx->CreateDebugMarker("Source Texture");

        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "colorFaces";

        static_assert((COLORFACES_NUMTHREAD_X == COLORFACES_NUMTHREAD_Y) && (COLORFACES_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = param.width / COLORFACES_NUMTHREAD_X;
        cmd.dispatch[1] = param.height / COLORFACES_NUMTHREAD_Y;
        cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / COLORFACES_NUMTHREAD_Z;

        cmd.uavBindings.insert(std::make_pair("dstTex", srcTex->uav[0]));

        dx->Dispatch(cmd);
    }

    // generate destination texture by sampling source using direction vectors
    {
        auto subMarker = dx->CreateDebugMarker("Destination Texture");

        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "dirToFaces";

        static_assert((DIRTOFACE_NUMTHREAD_X == DIRTOFACE_NUMTHREAD_Y) && (DIRTOFACE_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = param.width / DIRTOFACE_NUMTHREAD_X;
        cmd.dispatch[1] = param.height / DIRTOFACE_NUMTHREAD_Y;
        cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / DIRTOFACE_NUMTHREAD_Z;

        cmd.ssBindings.insert(std::make_pair("ssPoint", dx->GetSampler(ninniku::SS_Point)));
        cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvCube));
        cmd.uavBindings.insert(std::make_pair("dstTex", dstTex->uav[0]));

        dx->Dispatch(cmd);
    }

    auto srcImg = std::make_unique<ninniku::ddsImage>();

    srcImg->InitializeFromTextureObject(dx, srcTex);

    auto srcData = srcImg->GetData();
    auto srcHash = GetMD5(std::get<0>(srcData), std::get<1>(srcData));
    auto dstImg = std::make_unique<ninniku::ddsImage>();

    dstImg->InitializeFromTextureObject(dx, srcTex);

    auto dstData = dstImg->GetData();
    auto dstHash = GetMD5(std::get<0>(dstData), std::get<1>(dstData));

    BOOST_TEST(memcmp(srcHash, dstHash, sizeof(uint64_t) * 2) == 0);
}

BOOST_AUTO_TEST_CASE(shader_genMips)
{
    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("Generate Mips");

    auto image = std::make_unique<ninniku::ddsImage>();

    image->Load("data/Cathedral01.dds");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);

    ninniku::TextureParam param = {};
    param.format = srcParam.format;
    param.width = srcParam.width;
    param.height = srcParam.height;
    param.numMips = ninniku::CountMips(std::min(param.width, param.height));
    param.arraySize = ninniku::CUBEMAP_NUM_FACES;
    param.viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto resTex = dx->CreateTexture(param);

    // copy srcTex mip 0 to resTex
    {
        auto subMarker = dx->CreateDebugMarker("Copy mip 0");

        ninniku::CopySubresourceParam copyParams = {};

        copyParams.src = srcTex.get();
        copyParams.dst = resTex.get();

        for (uint16_t i = 0; i < ninniku::CUBEMAP_NUM_FACES; ++i) {
            copyParams.srcFace = i;
            copyParams.dstFace = i;

            dx->CopySubresource(copyParams);
        }
    }

    // mip generation
    {
        auto subMarker = dx->CreateDebugMarker("Generate remaining mips");

        for (uint32_t srcMip = 0; srcMip < param.numMips - 1; ++srcMip) {
            // dispatch
            ninniku::Command cmd = {};
            cmd.shader = "downsample";
            cmd.cbufferStr = "CBGlobal";

            static_assert((DOWNSAMPLE_NUMTHREAD_X == DOWNSAMPLE_NUMTHREAD_Y) && (DOWNSAMPLE_NUMTHREAD_Z == 1));
            cmd.dispatch[0] = std::max(1u, (param.width >> srcMip) / DOWNSAMPLE_NUMTHREAD_X);
            cmd.dispatch[1] = std::max(1u, (param.height >> srcMip) / DOWNSAMPLE_NUMTHREAD_Y);
            cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / DOWNSAMPLE_NUMTHREAD_Z;

            cmd.srvBindings.insert(std::make_pair("srcMip", resTex->srvArray[srcMip]));
            cmd.uavBindings.insert(std::make_pair("dstMipSlice", resTex->uav[srcMip + 1]));
            cmd.ssBindings.insert(std::make_pair("ssPoint", dx->GetSampler(ninniku::SS_Point)));

            // constant buffer
            CBGlobal cb = {};

            cb.targetMip = srcMip;
            dx->UpdateConstantBuffer(cmd.cbufferStr, &cb, sizeof(CBGlobal));

            dx->Dispatch(cmd);
        }
    }

    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, resTex);

    auto data = res->GetData();

    // note that WARP rendering cannot correctly run the mip generation phase so this hash it not entirely correct
    CheckMD5(std::get<0>(data), std::get<1>(data), 0x3a04d2be1d8de890, 0x551c8fa6af29a0fd);
}

BOOST_AUTO_TEST_CASE(shader_resize)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/Cathedral01.hdr");

    auto needFix = image->IsRequiringFix();
    auto newSize = std::get<1>(needFix);

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("Resize");
    auto srcTex = dx->CreateTexture(srcParam);

    ninniku::TextureParam dstParam = {};
    dstParam.width = newSize;
    dstParam.height = newSize;
    dstParam.format = srcTex->desc.format;
    dstParam.numMips = 1;
    dstParam.arraySize = 6;
    dstParam.viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    // dispatch
    ninniku::Command cmd = {};
    cmd.shader = "resize";
    cmd.ssBindings.insert(std::make_pair("ssLinear", dx->GetSampler(ninniku::SS_Linear)));
    cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvArray[0]));
    cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[0]));

    static_assert((RESIZE_NUMTHREAD_X == RESIZE_NUMTHREAD_Y) && (RESIZE_NUMTHREAD_Z == 1));
    cmd.dispatch[0] = cmd.dispatch[1] = newSize / RESIZE_NUMTHREAD_X;
    cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / RESIZE_NUMTHREAD_Z;

    dx->Dispatch(cmd);

    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, dst);

    auto data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x0a8789456200fe0c, 0x0801bef40235cedb);
}

BOOST_AUTO_TEST_SUITE_END()