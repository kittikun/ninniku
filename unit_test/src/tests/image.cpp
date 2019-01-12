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
#include "../common.h"

#include <boost/test/unit_test.hpp>
#include <ninniku/dx11/DX11.h>
#include <ninniku/image/cmft.h>
#include <ninniku/image/dds.h>
#include <ninniku/Image/generic.h>
#include <ninniku/ninniku.h>
#include <ninniku/types.h>

BOOST_AUTO_TEST_SUITE(Image)

BOOST_AUTO_TEST_CASE(cmft_load)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0xd39b5bad561c83d3, 0x585a996223bd1765);
}

BOOST_AUTO_TEST_CASE(cmft_from_texture_object)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/Cathedral01.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    auto data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x3da2a6a5fa290619, 0xd219e8a635672d15);
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

    auto param = image->CreateTextureParam(ninniku::TV_SRV);

    BOOST_TEST(param->arraySize == 6);
    BOOST_TEST(param->depth == 1);
    BOOST_TEST(param->format == DXGI_FORMAT_R32G32B32A32_FLOAT);
    BOOST_TEST(param->height == 512);
    BOOST_TEST(param->numMips == 1);
    BOOST_TEST(param->viewflags == ninniku::TV_SRV);
    BOOST_TEST(param->width == 512);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    std::string filename = "cmft_saveImage.dds";

    BOOST_TEST(image->SaveImageCubemap(filename, DXGI_FORMAT_R32G32B32A32_FLOAT));
    BOOST_TEST(boost::filesystem::exists(filename));

    CheckFileMD5(filename, 0x62a804a10dedbe15, 0xdcf18df4c67beda7);
}

BOOST_AUTO_TEST_CASE(cmft_saveImageFaceList)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    BOOST_TEST(image->SaveImageFaceList("cmft_saveImageFace", DXGI_FORMAT_R32G32B32A32_FLOAT));

    std::array<std::string, ninniku::CUBEMAP_NUM_FACES> suffixes = { "negx", "negy", "negz", "posx", "posy", "posz" };
    std::array<uint64_t, ninniku::CUBEMAP_NUM_FACES * 2> hashes = {
        0xf7013ee5b23c35ec, 0x2306cbcf87ed72fa,
        0x24954fe70382d69d, 0x057fa4a570e5d4e6,
        0x6e160948a9b88224, 0x47b58686fa530e9c,
        0x76b52949fc7534b8, 0xc7d97ddf7931834f,
        0x491744857fbacde7, 0x196987a132477a4c,
        0xd4d16960ed5c53ef, 0x4efd2157bdf514d6
    };

    auto basePath(boost::filesystem::current_path());

    for (auto i = 0; i < suffixes.size(); ++i) {
        auto filename = "cmft_saveImageFace_" + suffixes[i] + ".dds";

        BOOST_TEST(boost::filesystem::exists(filename));
        CheckFileMD5(filename, hashes[i * 2], hashes[i * 2 + 1]);
    }
}

BOOST_AUTO_TEST_CASE(dds_load)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto data = image->GetData();

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

    auto param = image->CreateTextureParam(ninniku::TV_SRV);

    BOOST_TEST(param->arraySize == 6);
    BOOST_TEST(param->depth == 1);
    BOOST_TEST(param->format == DXGI_FORMAT_R32G32B32A32_FLOAT);
    BOOST_TEST(param->height == 512);
    BOOST_TEST(param->numMips == 1);
    BOOST_TEST(param->viewflags == ninniku::TV_SRV);
    BOOST_TEST(param->width == 512);
}

BOOST_AUTO_TEST_CASE(dds_from_texture_object)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/Cathedral01.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    auto data = res->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x3da2a6a5fa290619, 0xd219e8a635672d15);
}

BOOST_AUTO_TEST_CASE(dds_saveImage_bc1)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc1.dds";

    BOOST_TEST(res->SaveImage(filename, dx, DXGI_FORMAT_BC1_UNORM));
    BOOST_TEST(boost::filesystem::exists(filename));

    CheckFileMD5(filename, 0xc7f0dc21e85e2395, 0xd5c963a78b66a4a4);
}

BOOST_AUTO_TEST_CASE(dds_saveImage_bc3)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/Rainbow_to_alpha_gradient.png"));

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc3.dds";

    BOOST_TEST(res->SaveImage(filename, dx, DXGI_FORMAT_BC3_UNORM));
    BOOST_TEST(boost::filesystem::exists(filename));

    CheckFileMD5(filename, 0x99fce9d6ec4ded22, 0x9bc0dedb31b4da7b);
}

BOOST_AUTO_TEST_CASE(dds_saveImage_bc4)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/toshi-1072059-unsplash.png"));

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc3.dds";

    BOOST_TEST(res->SaveImage(filename, dx, DXGI_FORMAT_BC4_UNORM));
    BOOST_TEST(boost::filesystem::exists(filename));

    // for some reason BC4 leads to different results between debug and release builds
#ifdef _DEBUG
    CheckFileMD5(filename, 0x397286da7060fabd, 0xb36a91e9ee1c3620);
#else
    CheckFileMD5(filename, 0x7131b97822cbd2e5, 0x9b68b7bc18d5a9fe);
#endif
}

BOOST_AUTO_TEST_CASE(dds_saveImage_bc5)
{
    auto image = std::make_unique<ninniku::genericImage>();

    image->Load("data/CC0-compressed-rock-NRM.png");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);

    // normal to derivative
    auto dstParam = ninniku::CreateEmptyTextureParam();
    dstParam->width = srcParam->width;
    dstParam->height = srcParam->height;
    dstParam->depth = srcParam->depth;
    dstParam->format = DXGI_FORMAT_R8G8_UNORM;
    dstParam->numMips = srcParam->numMips;
    dstParam->arraySize = srcParam->arraySize;
    dstParam->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    // dispatch
    ninniku::Command cmd = {};
    cmd.shader = "packNormals";
    cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvDefault));
    cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[0]));

    cmd.dispatch[0] = dstParam->width / PACKNORMALS_NUMTHREAD_X;
    cmd.dispatch[1] = dstParam->height / PACKNORMALS_NUMTHREAD_Y;
    cmd.dispatch[2] = PACKNORMALS_NUMTHREAD_Z;

    dx->Dispatch(cmd);

    auto res = std::make_unique<ninniku::ddsImage>();
    std::string filename = "dds_saveImage_bc5.dds";

    res->InitializeFromTextureObject(dx, dst);
    BOOST_TEST(res->SaveImage(filename, dx, DXGI_FORMAT_BC5_UNORM));

    auto basePath(boost::filesystem::current_path());
    auto path = basePath / filename;

    BOOST_TEST(boost::filesystem::exists(path));

#ifdef _DEBUG
    CheckFileMD5(path, 0xf93c2d7e95db8dd7, 0xd2895e64d48c6168);
#else
    CheckFileMD5(path, 0xab4fb70bf9b349f1, 0x9e5495233f1bf35d);
#endif
}

BOOST_AUTO_TEST_CASE(dds_saveImage_bc6h)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, srcTex);

    std::string filename = "dds_saveImage_bc6h.dds";

    BOOST_TEST(res->SaveImage(filename, dx, DXGI_FORMAT_BC6H_UF16));
    BOOST_TEST(boost::filesystem::exists(filename));

    CheckFileMD5(filename, 0x4a21b5bfd91ee91b, 0x046011be19fbd693);
}

BOOST_AUTO_TEST_CASE(dds_saveImage_bc7)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix);
    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, resized);

    std::string filename = "dds_saveImage_bc7.dds";

    BOOST_TEST(res->SaveImage(filename, dx, DXGI_FORMAT_BC7_UNORM));
    BOOST_TEST(boost::filesystem::exists(filename));

    CheckFileMD5(filename, 0x83dbc545c0057bef, 0x81e8e8c2154326bf);
}

BOOST_AUTO_TEST_CASE(generic_load)
{
    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_TEST(image->Load("data/banner.png"));

    auto data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x5c284747dea82181, 0xcdc216b5cbc13d95);

    BOOST_TEST(image->Load("data/architecture-buildings-city-1769347.jpg"));
    data = image->GetData();
    CheckMD5(std::get<0>(data), std::get<1>(data), 0x68762d0598a19f79, 0xa183c7f8664ffd53);
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
}

BOOST_AUTO_TEST_SUITE_END()