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

struct IDxcBlobEncoding;
struct IDxcLibrary;

namespace hlsl
{
    // this is from DxilContainer.h
#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
            (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
            (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
        )

    enum DxilFourCC
    {
        DFCC_Container = DXIL_FOURCC('D', 'X', 'B', 'C'), // for back-compat with tools that look for DXBC containers
        DFCCresource_Def = DXIL_FOURCC('R', 'D', 'E', 'F'),
        DFCC_InputSignature = DXIL_FOURCC('I', 'S', 'G', '1'),
        DFCC_OutputSignature = DXIL_FOURCC('O', 'S', 'G', '1'),
        DFCC_PatchConstantSignature = DXIL_FOURCC('P', 'S', 'G', '1'),
        DFCC_ShaderStatistics = DXIL_FOURCC('S', 'T', 'A', 'T'),
        DFCC_ShaderDebugInfoDXIL = DXIL_FOURCC('I', 'L', 'D', 'B'),
        DFCC_ShaderDebugName = DXIL_FOURCC('I', 'L', 'D', 'N'),
        DFCC_FeatureInfo = DXIL_FOURCC('S', 'F', 'I', '0'),
        DFCC_PrivateData = DXIL_FOURCC('P', 'R', 'I', 'V'),
        DFCC_RootSignature = DXIL_FOURCC('R', 'T', 'S', '0'),
        DFCC_DXIL = DXIL_FOURCC('D', 'X', 'I', 'L'),
        DFCC_PipelineStateValidation = DXIL_FOURCC('P', 'S', 'V', '0'),
    };
}

namespace ninniku
{
    [[nodiscard]] IDxcLibrary* GetDXCLibrary();
    [[nodiscard]] bool ValidateDXCBlob(IDxcBlobEncoding* pBlob, IDxcLibrary* pLibrary);
} // namespace ninniku