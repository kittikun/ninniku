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

#include "window.h"

namespace ninniku
{
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // sort through and find what code to run for the message given
        switch (message) {
            // this message is read when the window is closed
            case WM_CLOSE:
            case WM_DESTROY:
            case WM_QUIT:
            {
                // close the application entirely
                PostQuitMessage(0);
                return 0;
            } break;
        }

        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    HWND MakeWindow(uint32_t width, uint32_t height, bool show)
    {
        WNDCLASSEX wcex = {};
        HINSTANCE hInst = GetModuleHandle(0);

        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInst;
        wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = L"NinnikuWindow";
        wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        RegisterClassEx(&wcex);

        auto hWnd = CreateWindowEx(NULL, L"NinnikuWindow", L"Ninniku", WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                   NULL, NULL, hInst, NULL);

        if (show && (hWnd != nullptr))
            ShowWindow(hWnd, SW_SHOWDEFAULT);

        return hWnd;
    }
} // nam