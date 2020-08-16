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

#include "../fixture.h"

#include <ninniku/core/renderer/renderdevice.h>

#include <ninniku/ninniku.h>
#include <ninniku/utils.h>

BOOST_AUTO_TEST_SUITE(Misc)

BOOST_FIXTURE_TEST_CASE_TEMPLATE(renderdevice_dx12_check_feature, T, FixturesAll, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    auto res = false;

    for (auto i = 0u; i < ninniku::DF_COUNT; ++i) {
        auto check = dx->CheckFeatureSupport(static_cast<ninniku::EDeviceFeature>(i), res);

        switch (dx->GetType()) {
            case ninniku::ERenderer::RENDERER_DX11:
            case ninniku::ERenderer::RENDERER_WARP_DX11:
                BOOST_REQUIRE(!check);
                break;

            case ninniku::ERenderer::RENDERER_DX12:
            case ninniku::ERenderer::RENDERER_WARP_DX12:
                BOOST_REQUIRE(check);
                break;

            default:
                throw std::exception("case should not happen");
                break;
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(renderdevice_swapchain, T, FixturesDX12All, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    ninniku::SwapchainParam param;

    param.bufferCount = 2;
    param.format = ninniku::ETextureFormat::TF_R8G8B8A8_UNORM;
    param.height = 768;
    param.width = 1024;
    param.hwnd = ninniku::MakeWindow(param.width, param.height, false);
    param.vsync = false;

    auto& dx = ninniku::GetRenderer();

    auto sc = dx->CreateSwapChain(param);

    BOOST_REQUIRE(sc);
    BOOST_REQUIRE(sc->GetRTCount() == 2);

    for (auto i = 0u; i < sc->GetRTCount(); ++i) {
        BOOST_REQUIRE(sc->GetRT(i) != nullptr);
    }
}

BOOST_AUTO_TEST_SUITE_END()