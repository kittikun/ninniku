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

#define DO_RENDER_DEVICE_TESTS 1

#if DO_RENDER_DEVICE_TESTS

#include "../check.h"
#include "../fixture.h"
#include "../utils.h"

#include <ninniku/core/image/dds.h>
#include <ninniku/core/renderer/renderdevice.h>
#include <ninniku/ninniku.h>
#include <ninniku/utils.h>

#include <DirectXColors.h>

BOOST_AUTO_TEST_SUITE(RenderDevice)

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
                throw std::exception("Case should not happen");
                break;
        }
    }
}

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

BOOST_FIXTURE_TEST_CASE_TEMPLATE(renderdevice_create_buffer, T, FixturesDX12All, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    Vertex triangleVertices[] =
    {
        { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    auto params = ninniku::BufferParam::Create();

    params->elementSize = sizeof(Vertex);
    params->numElements = 3;
    params->initData = triangleVertices;

    auto srcBuffer = dx->CreateBuffer(params);

    auto dstBuffer = dx->CreateBuffer(srcBuffer);

    auto& data = dstBuffer->GetData();

    switch (dx->GetType()) {
        case ninniku::ERenderer::RENDERER_DX11:
        case ninniku::ERenderer::RENDERER_DX12:
        case ninniku::ERenderer::RENDERER_WARP_DX11:
        case ninniku::ERenderer::RENDERER_WARP_DX12:
            CheckCRC(std::get<0>(data), std::get<1>(data), 3378637337);
            break;

        default:
            throw std::exception("Case should not happen");
            break;
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(renderdevice_clear_rendertarget, T, FixturesDX12All, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto param = ninniku::SwapchainParam::Create();

    param->bufferCount = 2;
    param->format = ninniku::EFormat::F_R8G8B8A8_UNORM;
    param->height = 768;
    param->width = 1024;
    param->hwnd = ninniku::MakeWindow(param->width, param->height, false);
    param->vsync = false;

    auto& dx = ninniku::GetRenderer();

    auto swapChain = dx->CreateSwapChain(param);

    auto bufferIndex = swapChain->GetCurrentBackBufferIndex();
    auto frameRT = swapChain->GetRT(bufferIndex);

    ninniku::ClearRenderTargetParam clearParam;

    clearParam.color = DirectX::Colors::Cyan;
    clearParam.dstRT = frameRT;
    clearParam.index = bufferIndex;

    BOOST_REQUIRE(dx->ClearRenderTarget(clearParam));
    BOOST_REQUIRE(dx->Present(swapChain));

    auto image = std::make_unique<ninniku::ddsImage>();

    image->InitializeFromSwapChain(dx, swapChain);

    std::string filename = "renderdevice_clear_rendertarget.dds";

    ChangeToOutDirectory(T::platform);

    BOOST_REQUIRE(image->SaveImage(filename));
    BOOST_REQUIRE(std::filesystem::exists(filename));

    CheckFileCRC(filename, 1491251387);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(renderdevice_create_input_layout, T, FixturesDX12All, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto& dx = ninniku::GetRenderer();

    ninniku::InputLayoutDesc desc;

    desc.name = "simple";

    desc.elements.resize(2);
    desc.elements[0].name = "POSITION";
    desc.elements[0].format = ninniku::F_R32G32B32_FLOAT;
    desc.elements[1].name = "COLOR";
    desc.elements[1].format = ninniku::F_R32G32B32A32_FLOAT;

    dx->RegisterInputLayout(desc);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(renderdevice_swapchain, T, FixturesDX12All, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto param = ninniku::SwapchainParam::Create();

    param->bufferCount = 2;
    param->format = ninniku::EFormat::F_R8G8B8A8_UNORM;
    param->height = 768;
    param->width = 1024;
    param->hwnd = ninniku::MakeWindow(param->width, param->height, false);
    param->vsync = false;

    auto& dx = ninniku::GetRenderer();

    auto swapChain = dx->CreateSwapChain(param);

    BOOST_REQUIRE(swapChain.get() != nullptr);
    BOOST_REQUIRE(swapChain->GetRTCount() == 2);

    for (auto i = 0u; i < swapChain->GetRTCount(); ++i) {
        BOOST_REQUIRE(swapChain->GetRT(i) != nullptr);
    }
}

BOOST_AUTO_TEST_SUITE_END()
#endif // DO_RENDER_DEVICE_TESTS