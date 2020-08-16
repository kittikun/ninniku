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

#include <ninniku/ninniku.h>
#include <ninniku/core/renderer/renderdevice.h>
#include <ninniku/core/renderer/rendergraph.h>
#include <ninniku/core/image/cmft.h>
#include <ninniku/core/image/dds.h>

#include <DirectXColors.h>

int main()
{
    if (!ninniku::Initialize(ninniku::ERenderer::RENDERER_DX12, ninniku::EInitializationFlags::IF_None, ninniku::ELogLevel::LL_FULL))
        return -1;

    auto scDesc = ninniku::SwapchainParam::Create();

    scDesc->bufferCount = 2;
    scDesc->format = ninniku::ETextureFormat::TF_R8G8B8A8_UNORM;
    scDesc->height = 768;
    scDesc->width = 1024;
    scDesc->hwnd = ninniku::MakeWindow(scDesc->width, scDesc->height, true);
    scDesc->vsync = false;

    auto& dx = ninniku::GetRenderer();

    std::vector<ninniku::RTVHandle> swapchainRTs;

    auto swapChain = dx->CreateSwapChain(scDesc);

    MSG msg;
    bool running = true;

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if ((msg.message == WM_QUIT) || (msg.message == WM_CLOSE)) {
            running = false;
        } else {
            auto bufferIndex = swapChain->GetCurrentBackBufferIndex();
            auto frameRT = swapChain->GetRT(bufferIndex);

            ninniku::ClearRenderTargetParam clearParam;

            clearParam.color = DirectX::Colors::Cyan;
            clearParam.dstRT = frameRT;
            clearParam.index = bufferIndex;

            if (!dx->ClearRenderTarget(clearParam))
                throw std::exception("failed to clear");

            if (!dx->Present(swapChain))
                throw std::exception("failed to present");
        }
    };

    auto image = std::make_unique<ninniku::ddsImage>();

    image->InitializeFromSwapChain(dx, swapChain);

    image->SaveImage("test.dds");

    ninniku::Terminate();
}