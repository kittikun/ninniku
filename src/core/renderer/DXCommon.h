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

#pragma once

namespace ninniku
{
    class DXCommon
    {
        // not supposed to be instanced
        DXCommon() = delete;
        ~DXCommon() = delete;

    public:
        template<typename T>
        static bool GetDXGIFactory(T** pFactory)
        {
            if (!pFactory)
                return false;

            *pFactory = nullptr;

            typedef HRESULT(WINAPI * pfn_CreateDXGIFactory1)(REFIID riid, _Out_ void** ppFactory);

            static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

            if (!s_CreateDXGIFactory1) {
                auto hModDXGI = LoadLibrary(L"dxgi.dll");
                if (!hModDXGI)
                    return false;

                s_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));

                if (!s_CreateDXGIFactory1)
                    return false;
            }

            return SUCCEEDED(s_CreateDXGIFactory1(IID_PPV_ARGS(pFactory)));
        }
    };
} // namespace ninniku