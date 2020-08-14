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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(misc_dx12_check_feature, T, FixturesDX12, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    BOOST_REQUIRE(dx->CheckFeatureSupport(ninniku::EDeviceFeature::DF_NONE));
    BOOST_REQUIRE(dx->CheckFeatureSupport(ninniku::EDeviceFeature::DF_SM6_WAVE_INTRINSICS));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(misc_swapchain, T, FixturesAll, T)
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

    auto& dx = ninniku::GetRenderer();

    BOOST_REQUIRE(dx->CreateSwapChain(param));
}

BOOST_AUTO_TEST_SUITE_END()