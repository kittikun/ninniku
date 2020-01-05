// Copyright(c) 2018-2020 Kitti Vongsay
//
// Permission is hereby granted: free of charge: to any person obtaining a copy
// of this software and associated documentation files(the "Software"): to deal
// in the Software without restriction: including without limitation the rights
// to use: copy: modify: merge: publish: distribute: sublicense: and/or sell
// copies of the Software: and to permit persons to whom the Software is
// furnished to do so: subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS": WITHOUT WARRANTY OF ANY KIND: EXPRESS OR
// IMPLIED: INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY:
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM: DAMAGES OR OTHER
// LIABILITY: WHETHER IN AN ACTION OF CONTRACT: TORT OR OTHERWISE: ARISING FROM:
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "pch.h"
#include "misc.h"

#include <windows.h>

namespace ninniku
{
    constexpr const uint32_t DXGIFormatToNinnikuTF(uint32_t fmt)
    {
        ETextureFormat res = TF_UNKNOWN;

        switch (fmt) {
            case DXGI_FORMAT_UNKNOWN:
                res = TF_UNKNOWN;
                break;

            case DXGI_FORMAT_R8_UNORM:
                res = TF_R8_UNORM;
                break;
            case DXGI_FORMAT_R8G8_UNORM:
                res = TF_R8G8_UNORM;
                break;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                res = TF_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_R11G11B10_FLOAT:
                res = TF_R11G11B10_FLOAT;
                break;
            case DXGI_FORMAT_R16_UNORM:
                res = TF_R16_UNORM;
                break;
            case DXGI_FORMAT_R16G16_UNORM:
                res = TF_R16G16_UNORM;
                break;
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                res = TF_R16G16B16A16_FLOAT;
                break;
            case DXGI_FORMAT_R16G16B16A16_UNORM:
                res = TF_R16G16B16A16_UNORM;
                break;
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                res = TF_R32G32B32A32_FLOAT;
                break;

            default:
                throw std::exception("DXGIFormatToNinnikuTF unknown format");
        }

        return res;
    }

    constexpr const uint32_t NinnikuTFToDXGIFormat(uint32_t fmt)
    {
        DXGI_FORMAT res = DXGI_FORMAT_UNKNOWN;

        switch (fmt) {
            case TF_UNKNOWN:
                res = DXGI_FORMAT_UNKNOWN;
                break;

            case TF_R8_UNORM:
                res = DXGI_FORMAT_R8_UNORM;
                break;
            case TF_R8G8_UNORM:
                res = DXGI_FORMAT_R8G8_UNORM;
                break;
            case TF_R8G8B8A8_UNORM:
                res = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case TF_R11G11B10_FLOAT:
                res = DXGI_FORMAT_R11G11B10_FLOAT;
                break;
            case TF_R16_UNORM:
                res = DXGI_FORMAT_R16_UNORM;
                break;
            case TF_R16G16_UNORM:
                res = DXGI_FORMAT_R16G16_UNORM;
                break;
            case TF_R16G16B16A16_FLOAT:
                res = DXGI_FORMAT_R16G16B16A16_FLOAT;
                break;

            case TF_R16G16B16A16_UNORM:
                res = DXGI_FORMAT_R16G16B16A16_UNORM;
                break;
            case TF_R32G32B32A32_FLOAT:
                res = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;

            default:
                throw std::exception("NinnikuTFToDXGIFormat unknown format");
        }

        return res;
    }

    const std::string wstrToStr(const std::wstring& wstr)
    {
        std::string res;

        if (!wstr.empty()) {
            int needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
            res.resize(needed, 0);

            WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &res[0], needed, nullptr, nullptr);
        }

        return res;
    }

    const std::wstring strToWStr(const std::string& str)
    {
        std::wstring res;

        if (!str.empty()) {
            int needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
            res.resize(needed, 0);

            MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &res[0], needed);
        }

        return res;
    }
} // namespace ninniku