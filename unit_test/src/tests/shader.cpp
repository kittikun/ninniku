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

#include "../shaders/cbuffers.h"
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
    auto& dx = ninniku::GetRenderer();
    auto resTex = GenerateColoredMips(dx);
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, resTex);

    auto& data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x91086088d369be49, 0x74d54476510012cc);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_cubemapDirToArray, T, FixturesDX11, T)
{
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

    auto srcData = srcImg->GetData();
    auto srcHash = GetMD5(std::get<0>(srcData), std::get<1>(srcData));
    auto dstImg = std::make_unique<ninniku::ddsImage>();

    dstImg->InitializeFromTextureObject(dx, srcTex);

    auto dstData = dstImg->GetData();
    auto dstHash = GetMD5(std::get<0>(dstData), std::get<1>(dstData));

    BOOST_TEST(memcmp(srcHash, dstHash, sizeof(uint64_t) * 2) == 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_genMips, T, FixturesDX11, T)
{
    auto& dx = ninniku::GetRenderer();
    auto image = std::make_unique<ninniku::ddsImage>();

    image->Load("data/Cathedral01.dds");

    auto resTex = Generate2DTexWithMips(dx, image.get());
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, resTex);

    auto& data = res->GetData();

    // note that WARP rendering cannot correctly run the mip generation phase so this hash it not entirely correct
    CheckMD5(std::get<0>(data), std::get<1>(data), 0xc85514693c51df6f, 0xd10b1b7b4175a5ff);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_resize, T, FixturesDX11, T)
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

    CheckMD5(std::get<0>(data), std::get<1>(data), 0xb3ba50ac382fe166, 0xdd1bda49f1b43409);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_structuredBuffer, T, FixturesAll, T)
{
    // there is an error with AppVeyor CI with DX12 so disable it for now
    if (strcmp(std::getenv("APPVEYOR"), "True") == 0)
        return;

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

    CheckMD5(reinterpret_cast<uint8_t*>(const_cast<uint32_t*>(&data.front())), static_cast<uint32_t>(data.size() * sizeof(uint32_t)), 0xe4c6bd586aa54c9b, 0xb02b6bb8ec6b10db);
}

BOOST_AUTO_TEST_SUITE_END()