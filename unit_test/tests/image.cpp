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

#include <ninniku/dx11/DX11Types.h>
#include <ninniku/image/cmft.h>
#include <ninniku/types.h>
#include <tuple>

BOOST_AUTO_TEST_SUITE(Image)

BOOST_AUTO_TEST_CASE(load_cmft)
{
    SetupFixture f;
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_TEST(image->Load("data/whipple_creek_regional_park_01_2k.hdr"));
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

BOOST_AUTO_TEST_SUITE_END()