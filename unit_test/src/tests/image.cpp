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

#include "../check.h"

#include <boost/test/unit_test.hpp>
#include <ninniku/dx11/DX11.h>
#include <ninniku/image/cmft.h>
#include <ninniku/image/dds.h>
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
}

BOOST_AUTO_TEST_CASE(cmft_texture_parm)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto param = image->CreateTextureParam(ninniku::TV_SRV);

    BOOST_TEST(param.viewflags);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    BOOST_TEST(image->SaveImage("cmft_saveImage"));

    auto basePath(boost::filesystem::current_path());
    auto path = basePath / "cmft_saveImage.dds";

    BOOST_TEST(boost::filesystem::exists(path));

    CheckFileMD5(path, 0x62a804a10dedbe15, 0xdcf18df4c67beda7);
}

BOOST_AUTO_TEST_CASE(cmft_saveImageFaceList)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    BOOST_TEST(image->SaveImageFaceList("cmft_saveImageFace"));

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
        auto fileName = "cmft_saveImageFace_" + suffixes[i] + ".dds";
        auto path = basePath / fileName;

        BOOST_TEST(boost::filesystem::exists(path));
        CheckFileMD5(path, hashes[i * 2], hashes[i * 2 + 1]);
    }
}

BOOST_AUTO_TEST_CASE(dds_load)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x48e0c9680b2cbcc9, 0xea5f523bdad5cec4);
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

BOOST_AUTO_TEST_CASE(dds_saveImage)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);

    auto res = std::make_unique<ninniku::ddsImage>();

    res->InitializeFromTextureObject(dx, srcTex);
    BOOST_TEST(res->SaveImage("dds_saveImage.dds", dx, DXGI_FORMAT_BC6H_UF16));

    auto basePath(boost::filesystem::current_path());
    auto path = basePath / "dds_saveImage.dds";

    BOOST_TEST(boost::filesystem::exists(path));

    CheckFileMD5(path, 0x4a21b5bfd91ee91b, 0x046011be19fbd693);
}

BOOST_AUTO_TEST_SUITE_END()