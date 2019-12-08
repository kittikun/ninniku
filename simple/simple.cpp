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
#include <ninniku/renderer/dx11/DX11.h>
#include <ninniku/core/buffer.h>

int main()
{
    std::vector<std::string> shaderPaths = { "..\\simple\\shaders", "..\\unit_test\\shaders" };

    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX11, shaderPaths, ninniku::ELogLevel::LL_FULL);

    auto& dx = ninniku::GetRenderer();
    auto params = ninniku::BufferParam::Create();

    params->numElements = 16;
    params->elementSize = sizeof(uint32_t);
    params->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto srcBuffer = dx->CreateBuffer(params);

    // fill structured buffer
    {
        auto subMarker = dx->CreateDebugMarker("Fill StructuredBuffer");

        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "fillBuffer";

        cmd.dispatch[0] = cmd.dispatch[1] = cmd.dispatch[2] = 1;

        cmd.uavBindings.insert(std::make_pair("dstBuffer", srcBuffer->uav));

        dx->Dispatch(cmd);
    }

    ninniku::Buffer dstBuffer;

    dstBuffer.InitializeFromBufferObject(dx, srcBuffer);

    auto& data = dstBuffer.GetData();

    ninniku::Terminate();
}