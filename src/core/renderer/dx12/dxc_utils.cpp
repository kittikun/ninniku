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

#include "dxc_utils.h"

#include "../../../utils/misc.h"
#include "../../../utils/log.h"

#include <wrl/client.h>
#include <dxcapi.h>

namespace ninniku
{
    constexpr hlsl::DxilFourCC ShaderTypeToDXILFourCC(EShaderType type)
    {
        switch (type) {
            case ninniku::ST_Root_Signature:
                return hlsl::DFCC_RootSignature;
                break;

            default:
                return hlsl::DxilFourCC::DFCC_DXIL;
                break;
        }
    }

    bool IsTypeMatching(IDxcBlobEncoding* pBlob, EShaderType type, bool& result)
    {
        Microsoft::WRL::ComPtr<IDxcContainerReflection> pContainerReflection;

        auto hr = DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)&pContainerReflection);

        if (CheckAPIFailed(hr, "DxcCreateInstance for CLSID_DxcContainerReflection"))
            return false;

        hr = pContainerReflection->Load(pBlob);

        if (CheckAPIFailed(hr, "IDxcContainerReflection::Load"))
            return false;

        uint32_t partCount;

        hr = pContainerReflection->GetPartCount(&partCount);

        if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartCount"))
            return false;

        auto wanted4CC = ShaderTypeToDXILFourCC(type);

        for (uint32_t i = 0; i < partCount; ++i) {
            uint32_t partKind;

            hr = pContainerReflection->GetPartKind(i, &partKind);

            if (CheckAPIFailed(hr, "IDxcContainerReflection::GetPartKind"))
                return false;

            if (partKind == wanted4CC) {
                result = true;
                return true;
            }
        }

        result = false;
        return true;
    }

    IDxcLibrary* GetDXCLibrary()
    {
        static IDxcLibrary* pLibrary = nullptr;

        if (pLibrary == nullptr) {
            auto hr = DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary);

            if (CheckAPIFailed(hr, "DxcCreateInstance for CLSID_DxcLibrary"))
                return nullptr;
        }

        return pLibrary;
    }

    bool ValidateDXCBlob(IDxcBlobEncoding* pBlob, IDxcLibrary* pLibrary)
    {
        IDxcValidator* pValidator = nullptr;

        auto hr = DxcCreateInstance(CLSID_DxcValidator, __uuidof(IDxcValidator), (void**)&pValidator);

        if (CheckAPIFailed(hr, "DxcCreateInstance for CLSID_DxcValidator"))
            return false;

        Microsoft::WRL::ComPtr<IDxcOperationResult> dxcRes;
        pValidator->Validate(pBlob, DxcValidatorFlags_InPlaceEdit, &dxcRes);

        dxcRes->GetStatus(&hr);

        if (FAILED(hr)) {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob, printBlobUtf8;
            dxcRes->GetErrorBuffer(&printBlob);
            pLibrary->GetBlobAsUtf8(printBlob.Get(), printBlobUtf8.GetAddressOf());

            LOGEF(boost::format("Failed to validate IDxcBlobEncoding with: %1%") % reinterpret_cast<const char*>(printBlobUtf8->GetBufferPointer()));

            return false;
        }

        return true;
    }
} // namespace ninniku