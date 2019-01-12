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

#include <ninniku/ninniku.h>
#include <ninniku/dx11/DX11.h>
#include <ninniku/image/dds.h>
#include <ninniku/Image/generic.h>

int main()
{
    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX11, "E:\\ninniku\\unit_test\\shaders", ninniku::ELogLevel::LL_FULL);

    auto image = std::make_unique<ninniku::genericImage>();

    image->Load("E:\\ninniku\\unit_test\\data\\weave_16.png");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);

    // packed normal
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

    cmd.dispatch[0] = dstParam->width / 32;
    cmd.dispatch[1] = dstParam->height / 32;
    cmd.dispatch[2] = 1;

    dx->Dispatch(cmd);
    ninniku::Terminate();
}