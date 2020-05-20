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

#include "pch.h"
#include "dxc_utils.h"

#include "../../../utils/misc.h"
#include "../../../utils/log.h"

#include <dxcapi.h>

namespace ninniku {
    // https://www.wihlidal.com/blog/pipeline/2018-09-16-dxil-signing-post-compile/
    struct DxilMinimalHeader
    {
        UINT32 four_cc;
        UINT32 hash_digest[4];
    };

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

    bool IsDXILSigned(void* buffer)
    {
        DxilMinimalHeader* header = reinterpret_cast<DxilMinimalHeader*>(buffer);
        bool has_digest = false;
        has_digest |= header->hash_digest[0] != 0x0;
        has_digest |= header->hash_digest[1] != 0x0;
        has_digest |= header->hash_digest[2] != 0x0;
        has_digest |= header->hash_digest[3] != 0x0;
        return has_digest;
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

            std::string errorString;
            if (printBlobUtf8) {
                errorString = reinterpret_cast<const char*>(printBlobUtf8->GetBufferPointer());
            }

            LOGEF(boost::format("Failed to validate IDxcBlobEncoding with: %1%") % errorString);

            return false;
        }

        return true;
    }
} // namespace ninniku