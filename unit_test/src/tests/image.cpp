//Copyright(c) 2018 - 2019 Kitti Vongsay
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include "../shaders/dispatch.h"
#include "../shaders/cbuffers.h"
#include "../check.h"
#include "../common.h"
#include "../fixture.h"
#include "../utils.h"

#include <boost/test/unit_test.hpp>
#include <ninniku/core/renderer/renderdevice.h>
#include <ninniku/core/image/cmft.h>
#include <ninniku/core/image/dds.h>
#include <ninniku/core/image/generic.h>
#include <ninniku/ninniku.h>
#include <ninniku/types.h>
#include <ninniku/utils.h>
#include <filesystem>

BOOST_AUTO_TEST_SUITE(Image)

BOOST_AUTO_TEST_CASE(cmft_load)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto& data = image->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 3196376208);

    BOOST_TEST(image->Load("data/park02.exr"));
    auto& data2 = image->GetData();
    CheckCRC(std::get<0>(data2), std::get<1>(data2), 2283193732);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(cmft_from_texture_object, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/Cathedral01.hdr"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, srcTex));

    auto& data = res->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 1279329145);
}

BOOST_AUTO_TEST_CASE(cmft_need_resize)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/Cathedral01.hdr"));

    auto needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix));
    BOOST_TEST(std::get<1>(needFix) == 512);
    BOOST_TEST(std::get<2>(needFix) == 512);
}

BOOST_AUTO_TEST_CASE(cmft_texture_param)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto param = image->CreateTextureParam(ninniku::RV_SRV);

    BOOST_TEST(param->arraySize == 6);
    BOOST_TEST(param->depth == 1);
    BOOST_TEST(param->format == ninniku::DXGIFormatToNinnikuTF(DXGI_FORMAT_R32G32B32A32_FLOAT));
    BOOST_TEST(param->height == 512);
    BOOST_TEST(param->numMips == 1);
    BOOST_TEST(param->viewflags == ninniku::RV_SRV);
    BOOST_TEST(param->width == 512);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage_cubemap)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    std::string filename = "cmft_saveImage_cubemap.dds";

    BOOST_TEST(image->SaveImage(filename, ninniku::cmftImage::SaveType::Cubemap));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileCRC(filename, 3235862832);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage_faceList)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    BOOST_TEST(image->SaveImage("cmft_saveImageFace.dds", ninniku::cmftImage::SaveType::Facelist));

    std::array<std::string, ninniku::CUBEMAP_NUM_FACES> suffixes = { "negx", "negy", "negz", "posx", "posy", "posz" };
    std::array<uint32_t, ninniku::CUBEMAP_NUM_FACES> hashes = {
        2760747179,
        1583253975,
        3324696265,
        1668495217,
        3330494877,
        3989661171
    };

    for (auto i = 0; i < suffixes.size(); ++i) {
        auto filename = "cmft_saveImageFace_" + suffixes[i] + ".dds";

        BOOST_TEST(std::filesystem::exists(filename));
        CheckFileCRC(filename, hashes[i]);
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(cmft_saveImage_latlong, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, srcTex));

    std::string filename = "cmft_saveImage_longlat.hdr";

    BOOST_TEST(res->SaveImage(filename, ninniku::cmftImage::SaveType::LatLong));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileCRC(filename, 2361495689);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage_vcross)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    std::string filename = "cmft_saveImage_vcross.dds";

    BOOST_TEST(image->SaveImage(filename, ninniku::cmftImage::SaveType::VCross));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileCRC(filename, 3752497809);
}

BOOST_AUTO_TEST_CASE(dds_load)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto& data = image->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 2638212697);
}

BOOST_AUTO_TEST_CASE(dds_need_resize)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix) == false);
    BOOST_TEST(std::get<1>(needFix) == 512);
    BOOST_TEST(std::get<2>(needFix) == 512);
}

BOOST_AUTO_TEST_CASE(dds_texture_param)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto param = image->CreateTextureParam(ninniku::RV_SRV);

    BOOST_TEST(param->arraySize == 6);
    BOOST_TEST(param->depth == 1);
    BOOST_TEST(param->format == ninniku::DXGIFormatToNinnikuTF(DXGI_FORMAT_R32G32B32A32_FLOAT));
    BOOST_TEST(param->height == 512);
    BOOST_TEST(param->numMips == 1);
    BOOST_TEST(param->viewflags == ninniku::RV_SRV);
    BOOST_TEST(param->width == 512);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_from_texture_object, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/Cathedral01.hdr"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, srcTex));

    auto& data = res->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 1279329145);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_raw_mips, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    auto& dx = ninniku::GetRenderer();

    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto resTex = Generate2DTexWithMips(dx, image.get(), T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resTex));

    std::string filename = "dds_saveImage_raw_mips.dds";

    BOOST_TEST(res->SaveImage(filename));
    BOOST_TEST(std::filesystem::exists(filename));
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 4173211496);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_raw_cube_mips, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    auto& dx = ninniku::GetRenderer();

    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto resTex = GenerateColoredMips(dx, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resTex));

    std::string filename = "dds_saveImage_raw_cube_mips.dds";

    BOOST_TEST(res->SaveImage(filename));
    BOOST_TEST(std::filesystem::exists(filename));

    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 567904825);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_raw_cube_array_mips, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    auto& dx = ninniku::GetRenderer();

    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto resTex = GenerateColoredCubeArrayMips(dx, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resTex));

    std::string filename = "dds_saveImage_raw_cube_array__mips.dds";

    // check we can save
    BOOST_TEST(res->SaveImage(filename));
    BOOST_TEST(std::filesystem::exists(filename));

    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 3517905);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc1, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    auto& dx = ninniku::GetRenderer();

    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resized));

    std::string filename = "dds_saveImage_bc1.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC1_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    // dx11 can use GPU compression while dx12 uses CPU since DirectXTex doesn't support it
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 409997713);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 1032956914);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc3, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/Rainbow_to_alpha_gradient.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resized));

    std::string filename = "dds_saveImage_bc3.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC3_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    // dx11 can use GPU compression while dx12 uses CPU since DirectXTex doesn't support it
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 2138065852);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 2051743166);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc4, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/toshi-1072059-unsplash.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resized));

    std::string filename = "dds_saveImage_bc4.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC4_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    // for some reason BC4 leads to different results between debug/release
    // dx11/dx12 is because of hardware/software compression
#ifdef _DEBUG
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
            CheckFileCRC(filename, 1002934303);
            break;

        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 1002934303);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 1228450784);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
#else
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 769240813);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 2703939131);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
#endif
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc5_8bit, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    BOOST_TEST(LoadShader(dx, "packNormals", T::shaderRoot));

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/weave_8.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);

    // packed normal
    auto dstParam = ninniku::TextureParam::Create();
    dstParam->width = srcParam->width;
    dstParam->height = srcParam->height;
    dstParam->depth = srcParam->depth;
    dstParam->format = ninniku::DXGIFormatToNinnikuTF(DXGI_FORMAT_R8G8_UNORM);
    dstParam->numMips = srcParam->numMips;
    dstParam->arraySize = srcParam->arraySize;
    dstParam->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    // dispatch
    auto cmd = dx->CreateCommand();
    cmd->shader = "packNormals";
    cmd->srvBindings.insert(std::make_pair("srcTex", srcTex->GetSRVDefault()));
    cmd->uavBindings.insert(std::make_pair("dstTex", dst->GetUAV(0)));

    cmd->dispatch[0] = dstParam->width / PACKNORMALS_NUMTHREAD_X;
    cmd->dispatch[1] = dstParam->height / PACKNORMALS_NUMTHREAD_Y;
    cmd->dispatch[2] = PACKNORMALS_NUMTHREAD_Z;

    BOOST_TEST(dx->Dispatch(cmd));

    auto res = std::make_unique<ninniku::ddsImage>();
    std::string filename = "dds_saveImage_bc5_8.dds";

    BOOST_TEST(res->InitializeFromTextureObject(dx, dst));
    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC5_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 923975562);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 3356479526);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc5_16bit, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    BOOST_TEST(LoadShader(dx, "packNormals", T::shaderRoot));

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/weave_16.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);

    // packed normal
    auto dstParam = ninniku::TextureParam::Create();
    dstParam->width = srcParam->width;
    dstParam->height = srcParam->height;
    dstParam->depth = srcParam->depth;
    dstParam->format = ninniku::DXGIFormatToNinnikuTF(DXGI_FORMAT_R8G8_UNORM);
    dstParam->numMips = srcParam->numMips;
    dstParam->arraySize = srcParam->arraySize;
    dstParam->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    // dispatch
    auto cmd = dx->CreateCommand();
    cmd->shader = "packNormals";
    cmd->srvBindings.insert(std::make_pair("srcTex", srcTex->GetSRVDefault()));
    cmd->uavBindings.insert(std::make_pair("dstTex", dst->GetUAV(0)));

    cmd->dispatch[0] = dstParam->width / PACKNORMALS_NUMTHREAD_X;
    cmd->dispatch[1] = dstParam->height / PACKNORMALS_NUMTHREAD_Y;
    cmd->dispatch[2] = PACKNORMALS_NUMTHREAD_Z;

    BOOST_TEST(dx->Dispatch(cmd));

    auto res = std::make_unique<ninniku::ddsImage>();
    std::string filename = "dds_saveImage_bc5_16.dds";

    BOOST_TEST(res->InitializeFromTextureObject(dx, dst));
    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC5_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    // for some reason BC5 (16 bit) leads to different results between debug and release builds
#ifdef _DEBUG
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 1319360950);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 3254186394);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
#else
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 3457244965);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 2118249824);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
#endif
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc6h, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, srcTex));

    std::string filename = "dds_saveImage_bc6h.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC6H_UF16));
    BOOST_TEST(std::filesystem::exists(filename));

    // dx11 can use GPU compression while dx12 uses CPU since DirectXTex doesn't support it
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX12:
        case ninniku::ERenderer::RENDERER_WARP_DX12:
            CheckFileCRC(filename, 4073542973);
            break;

        case ninniku::ERenderer::RENDERER_DX11:
            CheckFileCRC(filename, 842772517);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 842772517);
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc7, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
        return;
    }

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(res->InitializeFromTextureObject(dx, resized));

    std::string_view filename = "dds_saveImage_bc7.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC7_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    // dx11 can use GPU compression while dx12 uses CPU since DirectXTex doesn't support it
    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX12:
            CheckFileCRC(filename, 2657823934);
            break;

        case ninniku::ERenderer::RENDERER_DX11:
            CheckFileCRC(filename, 3153192394);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX11:
            CheckFileCRC(filename, 2046772967);
            break;

        case ninniku::ERenderer::RENDERER_WARP_DX12:
            throw new std::exception("Invalid test, shouldn't happen");
            break;
    }
}

BOOST_AUTO_TEST_CASE(generic_load)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto& data = image->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 2997017566);

    BOOST_TEST(image->Load("data/architecture-buildings-city-1769347.jpg"));
    auto& data2 = image->GetData();
    CheckCRC(std::get<0>(data2), std::get<1>(data2), 2282433845);

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));
    auto& data3 = image->GetData();
    CheckCRC(std::get<0>(data3), std::get<1>(data3), 3486869451);
}

BOOST_AUTO_TEST_CASE(generic_need_resize)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix));
    BOOST_TEST(std::get<1>(needFix) == 1024);
    BOOST_TEST(std::get<2>(needFix) == 2048);

    BOOST_TEST(image->Load("data/architecture-buildings-city-1769347.jpg"));
    needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix));
    BOOST_TEST(std::get<1>(needFix) == 2048);
    BOOST_TEST(std::get<2>(needFix) == 2048);

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));
    needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix) == false);
    BOOST_TEST(std::get<1>(needFix) == 2048);
    BOOST_TEST(std::get<2>(needFix) == 1024);
}

BOOST_AUTO_TEST_SUITE_END()