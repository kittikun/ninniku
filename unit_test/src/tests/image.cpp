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

#include "../shaders/cbuffers.h"
#include "../check.h"
#include "../common.h"
#include "../fixture.h"

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

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x13f4aafdebe25865, 0x7ba34b1487530781);

    image->Load("data/park02.exr");
    auto& data2 = image->GetData();
    CheckMD5(std::get<0>(data2), std::get<1>(data2), 0x7df5652cbaf3a5af, 0xf758a4c5d9f5b418);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(cmft_from_texture_object, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/Cathedral01.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    auto& data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0xe4b0b9443383639a, 0x1236acd08f0a5de7);
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

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    std::string filename = "cmft_saveImage_cubemap.dds";

    BOOST_TEST(image->SaveImage(filename, ninniku::cmftImage::SaveType::Cubemap));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0xd390897a261c6e6d, 0xa7c0fc8663cf6756);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage_faceList)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    BOOST_TEST(image->SaveImage("cmft_saveImageFace.dds", ninniku::cmftImage::SaveType::Facelist));

    std::array<std::string, ninniku::CUBEMAP_NUM_FACES> suffixes = { "negx", "negy", "negz", "posx", "posy", "posz" };
    std::array<uint64_t, ninniku::CUBEMAP_NUM_FACES * 2> hashes = {
        0xdc56b26bf003de44, 0x06c970d707c430b6,
        0x870b6392741412ab, 0x10fbb21a5578aa5f,
        0xe5aaa9a1b0aa8b65, 0x3e2db13cced7c072,
        0xc866115bfa171b17, 0xdd0d2aca5d914f85,
        0x44672d04d224b3b6, 0x5445be0b2f0e5ef6,
        0x92ddcb5da9e8b320, 0xa1dfbe7d24cf0c2e
    };

    for (auto i = 0; i < suffixes.size(); ++i) {
        auto filename = "cmft_saveImageFace_" + suffixes[i] + ".dds";

        BOOST_TEST(std::filesystem::exists(filename));
        CheckFileMD5(filename, hashes[i * 2], hashes[i * 2 + 1]);
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(cmft_saveImage_latlong, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    image->Load("data/Cathedral01.dds");

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    std::string filename = "cmft_saveImage_longlat.hdr";

    BOOST_TEST(res->SaveImage(filename, ninniku::cmftImage::SaveType::LatLong));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x0e595ac204b6395f, 0x40389132a20d31db);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage_vcross)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    std::string filename = "cmft_saveImage_vcross.dds";

    BOOST_TEST(image->SaveImage(filename, ninniku::cmftImage::SaveType::VCross));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x7b301fd498ff0aaf, 0x10cfb2d95c6fbb22);
}

BOOST_AUTO_TEST_CASE(dds_load)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto& data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x48e0c9680b2cbcc9, 0xea5f523bdad5cec4);
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_from_texture_object, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/Cathedral01.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    auto& data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0xe4b0b9443383639a, 0x1236acd08f0a5de7);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_raw_mips, T, FixturesDX11, T)
{
    auto& dx = ninniku::GetRenderer();
    auto image = std::make_unique<ninniku::ddsImage>();

    image->Load("data/Cathedral01.dds");

    auto resTex = Generate2DTexWithMips(dx, image.get());
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resTex);

    std::string filename = "dds_saveImage_raw_mips.dds";

    BOOST_TEST(res->SaveImage(filename));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x90d2840de5fc390c, 0xaafa055284578053);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_raw_cube_mips, T, FixturesDX11, T)
{
    auto& dx = ninniku::GetRenderer();
    auto resTex = GenerateColoredMips(dx);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resTex);

    std::string filename = "dds_saveImage_raw_cube_mips.dds";

    BOOST_TEST(res->SaveImage(filename));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x96fc6f64b6361b46, 0xbb4679a507b22fe8);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_raw_cube_array_mips, T, FixturesDX11, T)
{
    auto& dx = ninniku::GetRenderer();
    auto resTex = GenerateColoredCubeArrayMips(dx);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resTex);

    std::string filename = "dds_saveImage_raw_cube_array__mips.dds";

    // check we can save
    BOOST_TEST(res->SaveImage(filename));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x38e5f5708ca8f166, 0x3cb321201074c3f7);

    // and reload
    BOOST_TEST(res->Load(filename));

    auto& data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x3d6bccfb68bfe11f, 0x764deda9ade288b4);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc1, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc1.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC1_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0xc7f0dc21e85e2395, 0xd5c963a78b66a4a4);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc3, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/Rainbow_to_alpha_gradient.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc3.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC3_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x99fce9d6ec4ded22, 0x9bc0dedb31b4da7b);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc4, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/toshi-1072059-unsplash.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc3.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC4_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    // for some reason BC4 leads to different results between debug and release builds
#ifdef _DEBUG
    CheckFileMD5(filename, 0x397286da7060fabd, 0xb36a91e9ee1c3620);
#else
    CheckFileMD5(filename, 0x9935add6cf01cfb2, 0xf1cd1f43812c2962);
#endif
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc5_8bit, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::genericImage>();

    image->Load("data/weave_8.png");

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
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

    dx->Dispatch(cmd);

    auto res = std::make_unique<ninniku::ddsImage>();
    std::string filename = "dds_saveImage_bc5_8.dds";

    res->InitializeFromTextureObject(dx, dst);
    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC5_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

#ifdef _DEBUG
    CheckFileMD5(filename, 0x8338717097b81c8f, 0x96d43528fdcca03a);
#else
    CheckFileMD5(filename, 0x8338717097b81c8f, 0x96d43528fdcca03a);
#endif
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc5_16bit, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::genericImage>();

    image->Load("data/weave_16.png");

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
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

    dx->Dispatch(cmd);

    auto res = std::make_unique<ninniku::ddsImage>();
    std::string filename = "dds_saveImage_bc5_16.dds";

    res->InitializeFromTextureObject(dx, dst);
    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC5_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

#ifdef _DEBUG
    CheckFileMD5(filename, 0x41be141dec6447ee, 0xb881f3768608f0e1);
#else
    CheckFileMD5(filename, 0xfea9a5d1d0b1ec98, 0xcaf351a85cecc7cc);
#endif
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc6h, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    std::string filename = "dds_saveImage_bc6h.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC6H_UF16));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x4a21b5bfd91ee91b, 0x046011be19fbd693);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc7, T, FixturesDX11, T)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc7.dds";

    BOOST_TEST(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC7_UNORM));
    BOOST_TEST(std::filesystem::exists(filename));

    CheckFileMD5(filename, 0x83dbc545c0057bef, 0x81e8e8c2154326bf);
}

BOOST_AUTO_TEST_CASE(generic_load)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto& data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x5c284747dea82181, 0xcdc216b5cbc13d95);

    BOOST_TEST(image->Load("data/architecture-buildings-city-1769347.jpg"));
    auto& data2 = image->GetData();
    CheckMD5(std::get<0>(data2), std::get<1>(data2), 0x68762d0598a19f79, 0xa183c7f8664ffd53);

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));
    auto& data3 = image->GetData();
    CheckMD5(std::get<0>(data3), std::get<1>(data3), 0xbde7e6526b1c6f06, 0x87ac4825f91dc73b);
}

BOOST_AUTO_TEST_CASE(generic_need_resize)
{
    auto image = std::make_unique<ninniku::genericImage>();

    image->Load("data/banner.png");

    auto needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix));
    BOOST_TEST(std::get<1>(needFix) == 1024);
    BOOST_TEST(std::get<2>(needFix) == 2048);

    image->Load("data/architecture-buildings-city-1769347.jpg");
    needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix));
    BOOST_TEST(std::get<1>(needFix) == 2048);
    BOOST_TEST(std::get<2>(needFix) == 2048);

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");
    needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix) == false);
    BOOST_TEST(std::get<1>(needFix) == 2048);
    BOOST_TEST(std::get<2>(needFix) == 1024);
}

BOOST_AUTO_TEST_SUITE_END()