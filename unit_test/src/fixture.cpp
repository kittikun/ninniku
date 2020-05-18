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

#include "fixture.h"

#include "utils.h"

#include <ninniku/ninniku.h>

#include <boost/test/debug.hpp>

#ifdef _DEBUG
static constexpr std::string_view DX11ShadersRoot = "bin\\Debug\\dx11";
static constexpr std::string_view DX12ShadersRoot = "bin\\Debug\\dx12";
#else
static constexpr std::string_view DX11ShadersRoot = "bin\\Release\\dx11";
static constexpr std::string_view DX12ShadersRoot = "bin\\Release\\dx12";
#endif

SetupFixtureDX11::SetupFixtureDX11()
    : shaderRoot{ DX11ShadersRoot }
{
    auto renderer = ninniku::ERenderer::RENDERER_DX11;
    uint32_t flags = ninniku::EInitializationFlags::IF_BC7_QUICK_MODE;

    if (IsAppVeyor()) {
        isNull = true;
    }

    // because unit test run on CI, always use WARP
    if (!ninniku::Initialize(renderer, flags, ninniku::ELogLevel::LL_FULL)) {
        throw new std::exception("Failed to initialize Ninniku.");
    }
}

SetupFixtureDX11::~SetupFixtureDX11()
{
    ninniku::Terminate();
}

SetupFixtureDX11Warp::SetupFixtureDX11Warp()
    : shaderRoot{ DX11ShadersRoot }
{
    auto renderer = ninniku::ERenderer::RENDERER_WARP_DX11;
    uint32_t flags = ninniku::EInitializationFlags::IF_BC7_QUICK_MODE;

    // because unit test run on CI, always use WARP
    if (!ninniku::Initialize(renderer, flags, ninniku::ELogLevel::LL_FULL)) {
        throw new std::exception("Failed to initialize Ninniku.");
    }
}

SetupFixtureDX11Warp::~SetupFixtureDX11Warp()
{
    ninniku::Terminate();
}

SetupFixtureDX12::SetupFixtureDX12()
    : shaderRoot{ DX12ShadersRoot }
{
    auto renderer = ninniku::ERenderer::RENDERER_DX12;
    uint32_t flags = ninniku::EInitializationFlags::IF_BC7_QUICK_MODE;

    if (IsAppVeyor()) {
        isNull = true;
    }

    // because unit test run on CI, always use WARP
    if (!ninniku::Initialize(renderer, flags, ninniku::ELogLevel::LL_FULL)) {
        throw new std::exception("Failed to initialize Ninniku.");
    }
}

SetupFixtureDX12::~SetupFixtureDX12()
{
    ninniku::Terminate();
}

SetupFixtureDX12Warp::SetupFixtureDX12Warp()
    : shaderRoot{ DX12ShadersRoot }
{
    auto renderer = ninniku::ERenderer::RENDERER_WARP_DX12;
    uint32_t flags = ninniku::EInitializationFlags::IF_BC7_QUICK_MODE;

    if (IsAppVeyor()) {
        flags |= ninniku::EInitializationFlags::IF_DisableDX12DebugLayer;
    }

    // because unit test run on CI, always use WARP
    if (!ninniku::Initialize(renderer, flags, ninniku::ELogLevel::LL_FULL)) {
        throw new std::exception("Failed to initialize Ninniku.");
    }
}

SetupFixtureDX12Warp::~SetupFixtureDX12Warp()
{
    ninniku::Terminate();
}