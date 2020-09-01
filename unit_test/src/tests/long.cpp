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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/test/unit_test.hpp>
#pragma clang diagnostic pop

#define DO_LONG_TESTS 1

#if DO_LONG_TESTS

#include "../check.h"
#include "../common.h"
#include "../fixture.h"
#include "../utils.h"

#include <ninniku/core/image/cmft.h>
#include <ninniku/core/image/dds.h>
#include <ninniku/core/image/generic.h>
#include <ninniku/core/renderer/renderdevice.h>
#include <ninniku/ninniku.h>

BOOST_AUTO_TEST_SUITE(Long)

BOOST_FIXTURE_TEST_CASE(cmft_load, SetupFixtureNull)
{
    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_REQUIRE(image->Load("data/images/whipple_creek_regional_park_01_2k.hdr"));

    auto& data = image->GetData();

    CheckCRC(std::get<0>(data), std::get<1>(data), 3196376208);

    BOOST_REQUIRE(image->Load("data/images/park02.exr"));
    auto& data2 = image->GetData();
    CheckCRC(std::get<0>(data2), std::get<1>(data2), 2283193732);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc6h, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    // skip BC6H/BC7 compression on CI debug because it will make Appveyor timeout
#ifdef _DEBUG
    if (IsAppVeyor())
        return;
#endif

    auto image = std::make_unique<ninniku::cmftImage>();

    BOOST_REQUIRE(image->Load("data/images/whipple_creek_regional_park_01_2k.hdr"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_REQUIRE(res->InitializeFromTextureObject(dx, srcTex.get()));

    ChangeToOutDirectory(T::platform);

    std::string filename = "dds_saveImage_bc6h.dds";

    BOOST_REQUIRE(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC6H_UF16));
    BOOST_REQUIRE(std::filesystem::exists(filename));

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

    default:
        throw std::exception("Case should not happen");
        break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dds_saveImage_bc7, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    auto image = std::make_unique<ninniku::genericImage>();

    BOOST_REQUIRE(image->Load("data/images/banner.png"));

    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);
    auto needFix = image->IsRequiringFix();
    auto resized = ResizeImage(dx, srcTex, needFix, T::shaderRoot);
    auto res = std::make_unique<ninniku::ddsImage>();

    BOOST_REQUIRE(res->InitializeFromTextureObject(dx, resized.get()));

    ChangeToOutDirectory(T::platform);

    std::string_view filename = "dds_saveImage_bc7.dds";

    BOOST_REQUIRE(res->SaveCompressedImage(filename, dx, DXGI_FORMAT_BC7_UNORM));
    BOOST_REQUIRE(std::filesystem::exists(filename));

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
        CheckFileCRC(filename, 222998642);
        break;

    default:
        throw std::exception("Case should not happen");
        break;
    }
}

BOOST_AUTO_TEST_SUITE_END()
#endif // DO_LONG_TESTS