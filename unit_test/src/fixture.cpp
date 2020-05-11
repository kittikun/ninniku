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

#include <ninniku/ninniku.h>

SetupFixtureDX11::SetupFixtureDX11()
{
    // because unit test run on CI, always use WARP
#ifdef _DEBUG
    std::vector<std::string_view> shaderPaths = { "bin\\Debug\\dx11" };
#else
    std::vector<std::string_view> shaderPaths = { "bin\\Release\\dx11" };
#endif

    ninniku::Initialize(ninniku::ERenderer::RENDERER_WARP_DX11, shaderPaths, false, ninniku::ELogLevel::LL_FULL);
}

SetupFixtureDX11::~SetupFixtureDX11()
{
    ninniku::Terminate();
}

SetupFixtureDX12::SetupFixtureDX12()
{
    // because unit test run on CI, always use WARP
#ifdef _DEBUG
    std::vector<std::string_view> shaderPaths = { "bin\\Debug\\dx12" };
#else
    std::vector<std::string_view> shaderPaths = { "bin\\Release\\dx12" };
#endif

    // There is a problem with the WARP renderer and DX12 so all tests are to be disabled for Appveyor now..
    auto renderer = ninniku::ERenderer::RENDERER_WARP_DX12;

    ninniku::Initialize(renderer, shaderPaths, false, ninniku::ELogLevel::LL_FULL);
}

SetupFixtureDX12::~SetupFixtureDX12()
{
    ninniku::Terminate();
}

bool IsAppVeyor()
{
    char* value;
    size_t len;

    auto ret = _dupenv_s(&value, &len, "APPVEYOR");

    if ((len > 0) && (strcmp(value, "True") == 0))
        return true;

    return false;
}