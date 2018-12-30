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

#include <boost/test/unit_test.hpp>

#include "../fixture.h"

#include <boost/filesystem.hpp>
#include <ninniku/dx11/DX11Types.h>
#include <ninniku/image/cmft.h>
#include <ninniku/image/dds.h>
#include <ninniku/types.h>
#include <iostream>

BOOST_AUTO_TEST_SUITE(Image)

BOOST_AUTO_TEST_CASE(load_cmft)
{
    SetupFixture f;
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0xd39b5bad561c83d3, 0x585a996223bd1765);
}

BOOST_AUTO_TEST_CASE(cmft_need_resize)
{
    SetupFixture f;
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/Cathedral01.hdr"));

    auto needFix = image->IsRequiringFix();

    BOOST_TEST(std::get<0>(needFix));
    BOOST_TEST(std::get<1>(needFix) == 512);
}

BOOST_AUTO_TEST_CASE(cmft_texture_parm)
{
    SetupFixture f;
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));

    auto param = image->CreateTextureParam(ninniku::TV_SRV);

    BOOST_TEST(param.viewflags);
}

BOOST_AUTO_TEST_CASE(cmft_saveImage)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("data/whipple_creek_regional_park_01_2k.hdr");

    image->SaveImage("cmft_saveImage");

    auto basePath(boost::filesystem::current_path());
    auto path = basePath / "cmft_saveImage.dds";

    BOOST_TEST(boost::filesystem::exists(path));

    std::ifstream ifs(path.c_str(), std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<uint8_t> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(result.data()), pos);

    CheckMD5(result.data(), static_cast<uint32_t>(result.size()), 0x62a804a10dedbe15, 0xdcf18df4c67beda7);
}

BOOST_AUTO_TEST_CASE(dds_load)
{
    auto image = std::make_unique<ninniku::ddsImage>();

    BOOST_TEST(image->Load("data/Cathedral01.dds"));

    auto data = image->GetData();

    CheckMD5(std::get<0>(data), std::get<1>(data), 0x48e0c9680b2cbcc9, 0xea5f523bdad5cec4);
}

BOOST_AUTO_TEST_SUITE_END()