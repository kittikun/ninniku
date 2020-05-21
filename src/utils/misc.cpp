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

#include "ninniku/core/renderer/renderdevice.h"

#include "../core/renderer/dx11/DX11.h"
#include "../core/renderer/dx12/DX12.h"
#include "../globals.h"
#include "log.h"

#include <atlbase.h>
#include <comdef.h>
#include <windows.h>

namespace ninniku
{
    uint32_t Align(UINT uLocation, uint32_t uAlign)
    {
        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources
        if ((0 == uAlign) || (uAlign & (uAlign - 1))) {
            throw std::exception("Non-pow2 alignment");
        }

        return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
    }

    bool CheckAPIFailed(HRESULT hr, const std::string_view& apiName)
    {
        if (FAILED(hr)) {
            _com_error err(hr);
            LOGEF(boost::format("Failed to %1% with: %2%") % apiName % wstrToStr(err.ErrorMessage()));

            if (hr == DXGI_ERROR_DEVICE_REMOVED) {
                auto& renderer = GetRenderer();

                if ((renderer->GetType() & ERenderer::RENDERER_DX12) != 0) {
                    auto dx = static_cast<DX12*>(renderer.get());

                    if (Globals::Instance()._useDebugLayer) {
                        CComPtr<ID3D12DeviceRemovedExtendedData> pDred;
                        auto hr2 = dx->GetDevice()->QueryInterface(IID_PPV_ARGS(&pDred));

                        if (CheckAPIFailed(hr2, "ID3D12Device::QueryInterface"))
                            return true;

                        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dredAutoBreadcrumbsOutput;
                        D3D12_DRED_PAGE_FAULT_OUTPUT dredPageFaultOutput;

                        hr2 = pDred->GetAutoBreadcrumbsOutput(&dredAutoBreadcrumbsOutput);

                        if (CheckAPIFailed(hr2, "ID3D12DeviceRemovedExtendedData::GetAutoBreadcrumbsOutput"))
                            return true;

                        hr2 = pDred->GetPageFaultAllocationOutput(&dredPageFaultOutput);

                        if (CheckAPIFailed(hr2, "ID3D12DeviceRemovedExtendedData::GetPageFaultAllocationOutput"))
                            return true;

                        throw new std::exception("DRED parsing not yet implemented");
                    } else {
                        auto hr2 = dx->GetDevice()->GetDeviceRemovedReason();

                        if (CheckAPIFailed(hr2, "ID3D11Device::GetDeviceRemovedReason"))
                            return true;
                    }
                } else {
                    auto dx = static_cast<DX11*>(renderer.get());

                    auto hr2 = dx->GetDevice()->GetDeviceRemovedReason();

                    if (CheckAPIFailed(hr2, "ID3D11Device::GetDeviceRemovedReason"))
                        return true;
                }
            }

            return true;
        }

        return false;
    }

    constexpr const uint32_t DXGIFormatToNumBytes(uint32_t format)
    {
        // https://stackoverflow.com/questions/40339138/convert-dxgi-format-to-a-bpp
        uint32_t res = 0;

        switch (format) {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
            {
                res = 16;
                break;
            }

            case DXGI_FORMAT_R32G32B32_TYPELESS:
            case DXGI_FORMAT_R32G32B32_FLOAT:
            case DXGI_FORMAT_R32G32B32_UINT:
            case DXGI_FORMAT_R32G32B32_SINT:
            {
                res = 12;
                break;
            }

            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R16G16B16A16_SINT:
            case DXGI_FORMAT_R32G32_TYPELESS:
            case DXGI_FORMAT_R32G32_FLOAT:
            case DXGI_FORMAT_R32G32_UINT:
            case DXGI_FORMAT_R32G32_SINT:
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            case DXGI_FORMAT_Y416:
            case DXGI_FORMAT_Y210:
            case DXGI_FORMAT_Y216:
            {
                res = 8;
                break;
            }

            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R11G11B10_FLOAT:
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_R16G16_TYPELESS:
            case DXGI_FORMAT_R16G16_FLOAT:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_R32_SINT:
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            case DXGI_FORMAT_R8G8_B8G8_UNORM:
            case DXGI_FORMAT_G8R8_G8B8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            case DXGI_FORMAT_AYUV:
            case DXGI_FORMAT_Y410:
            case DXGI_FORMAT_YUY2:
            {
                res = 4;
                break;
            }

            case DXGI_FORMAT_R8G8_TYPELESS:
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8_UINT:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R16_SINT:
            case DXGI_FORMAT_B5G6R5_UNORM:
            case DXGI_FORMAT_B5G5R5A1_UNORM:
            case DXGI_FORMAT_A8P8:
            case DXGI_FORMAT_B4G4R4A4_UNORM:
            {
                res = 2;
                break;
            }

            case DXGI_FORMAT_R8_TYPELESS:
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8_UINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_A8_UNORM:
            case DXGI_FORMAT_AI44:
            case DXGI_FORMAT_IA44:
            case DXGI_FORMAT_P8:
            {
                res = 1;
                break;
            }

            // those are less than a byte per pixel so leave them out for now
            //case DXGI_FORMAT_BC1_TYPELESS:
            //case DXGI_FORMAT_BC1_UNORM:
            //case DXGI_FORMAT_BC1_UNORM_SRGB:
            //case DXGI_FORMAT_BC4_TYPELESS:
            //case DXGI_FORMAT_BC4_UNORM:
            //case DXGI_FORMAT_BC4_SNORM:
            //    return 0.5;

            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
            {
                res = 1;
                break;
            }

            default:
                throw std::exception("DXGIFormatToNumBytes unexpected format");
        }

        return res;
    }

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
            case DXGI_FORMAT_R32_FLOAT:
                res = TF_R32_FLOAT;
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
            case TF_R32_FLOAT:
                res = DXGI_FORMAT_R32_FLOAT;
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

    const std::wstring strToWStr(const std::string_view& str)
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