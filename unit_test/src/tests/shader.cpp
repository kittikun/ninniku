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

#include "../shaders/dx11/cbuffers.h"
#include "../shaders/dx12/cbuffers.h"
#include "../check.h"
#include "../common.h"
#include "../fixture.h"

#include <boost/test/unit_test.hpp>
#include <ninniku/core/renderer/renderdevice.h>
#include <ninniku/core/renderer/types.h>
#include <ninniku/core/image/cmft.h>
#include <ninniku/core/image/dds.h>
#include <ninniku/ninniku.h>
#include <ninniku/types.h>
#include <ninniku/utils.h>

BOOST_AUTO_TEST_SUITE(Shader)

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_colorMips, T, FixturesDX11, T)
{
    // DX12 doesn't allow binding the same resource as SRV and UAV so the shader needs to be rewritten later
    auto& dx = ninniku::GetRenderer();
    auto resTex = GenerateColoredMips(dx);
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, resTex);

    auto& data = res->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 3775864256);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_cubemapDirToArray, T, FixturesDX11, T)
{
    // There is something wrong with WARP but it's working fine for DX12 HW so disable it since unit tests are using WARP
    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("CubemapDirToArray");

    auto param = ninniku::TextureParam::Create();
    param->width = param->height = 512;
    param->format = ninniku::DXGIFormatToNinnikuTF(DXGI_FORMAT_R32G32B32A32_FLOAT);
    param->depth = 1;
    param->numMips = 1;
    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
    param->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto srcTex = dx->CreateTexture(param);
    auto dstTex = dx->CreateTexture(param);

    // generate source texture
    {
        auto subMarker = dx->CreateDebugMarker("Source Texture");

        // dispatch
        auto cmd = dx->CreateCommand();
        cmd->shader = "colorFaces";

        static_assert((COLORFACES_NUMTHREAD_X == COLORFACES_NUMTHREAD_Y) && (COLORFACES_NUMTHREAD_Z == 1));
        cmd->dispatch[0] = param->width / COLORFACES_NUMTHREAD_X;
        cmd->dispatch[1] = param->height / COLORFACES_NUMTHREAD_Y;
        cmd->dispatch[2] = ninniku::CUBEMAP_NUM_FACES / COLORFACES_NUMTHREAD_Z;

        cmd->uavBindings.insert(std::make_pair("dstTex", srcTex->GetUAV(0)));

        dx->Dispatch(cmd);
    }

    // generate destination texture by sampling source using direction vectors
    {
        auto subMarker = dx->CreateDebugMarker("Destination Texture");

        // dispatch
        auto cmd = dx->CreateCommand();
        cmd->shader = "dirToFaces";

        static_assert((DIRTOFACE_NUMTHREAD_X == DIRTOFACE_NUMTHREAD_Y) && (DIRTOFACE_NUMTHREAD_Z == 1));
        cmd->dispatch[0] = param->width / DIRTOFACE_NUMTHREAD_X;
        cmd->dispatch[1] = param->height / DIRTOFACE_NUMTHREAD_Y;
        cmd->dispatch[2] = ninniku::CUBEMAP_NUM_FACES / DIRTOFACE_NUMTHREAD_Z;

        cmd->ssBindings.insert(std::make_pair("ssPoint", dx->GetSampler(ninniku::ESamplerState::SS_Point)));
        cmd->srvBindings.insert(std::make_pair("srcTex", srcTex->GetSRVCube()));
        cmd->uavBindings.insert(std::make_pair("dstTex", dstTex->GetUAV(0)));

        dx->Dispatch(cmd);
    }

    auto srcImg = std::make_unique<ninniku::ddsImage>();

    srcImg->InitializeFromTextureObject(dx, srcTex);
    srcImg->SaveImage("shader_cubemapDirToArray_src.dds");

    auto srcData = srcImg->GetData();
    auto srcHash = GetCRC(std::get<0>(srcData), std::get<1>(srcData));
    auto dstImg = std::make_unique<ninniku::ddsImage>();

    dstImg->InitializeFromTextureObject(dx, srcTex);
    srcImg->SaveImage("shader_cubemapDirToArray_dst.dds");
    auto dstData = dstImg->GetData();
    auto dstHash = GetCRC(std::get<0>(dstData), std::get<1>(dstData));

    BOOST_TEST(srcHash == dstHash);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_genMips, T, FixturesDX11, T)
{
    // DX12 doesn't allow binding the same resource as SRV and UAV so the shader needs to be rewritten later
    auto& dx = ninniku::GetRenderer();
    auto image = std::make_unique<ninniku::ddsImage>();

    image->Load("data/Cathedral01.dds");

    auto resTex = Generate2DTexWithMips(dx, image.get());
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, resTex);

    auto& data = res->GetData();

    // note that WARP rendering cannot correctly run the mip generation phase so this hash it not entirely correct
    CheckCRC(std::get<0>(data), std::get<1>(data), 946385041);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_resize, T, FixturesAll, T)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/Cathedral01.hdr");

    auto needFix = image->IsRequiringFix();
    auto newSize = std::get<1>(needFix);

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("Resize");
    auto srcTex = dx->CreateTexture(srcParam);

    auto dstParam = ninniku::TextureParam::Create();
    dstParam->width = newSize;
    dstParam->height = newSize;
    dstParam->format = srcTex->GetDesc()->format;
    dstParam->numMips = 1;
    dstParam->arraySize = 6;
    dstParam->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto dst = ResizeImage(dx, srcTex, needFix);

    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, dst);

    auto& data = res->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 457450649);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_structuredBuffer, T, FixturesAll, T)
{
    auto& dx = ninniku::GetRenderer();
    auto params = ninniku::BufferParam::Create();

    params->numElements = 16;
    params->elementSize = sizeof(uint32_t);
    params->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto srcBuffer = dx->CreateBuffer(params);

    // fill structured buffer
    {
        auto subMarker = dx->CreateDebugMarker("Fill StructuredBuffer");

        // dispatch
        auto cmd = dx->CreateCommand();
        cmd->shader = "fillBuffer";

        cmd->dispatch[0] = cmd->dispatch[1] = cmd->dispatch[2] = 1;

        cmd->uavBindings.insert(std::make_pair("dstBuffer", srcBuffer->GetUAV()));

        dx->Dispatch(cmd);
    }

    auto dstBuffer = dx->CreateBuffer(srcBuffer);

    auto& data = dstBuffer->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 3783883977);
}

BOOST_AUTO_TEST_SUITE_END()