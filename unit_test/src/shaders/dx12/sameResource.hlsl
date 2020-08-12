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

#include"../cbuffers.h"
#include "../dispatch.h"

#define RS  "RootFlags( DENY_VERTEX_SHADER_ROOT_ACCESS | " \
                       "DENY_HULL_SHADER_ROOT_ACCESS | " \
                       "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                       "DENY_GEOMETRY_SHADER_ROOT_ACCESS | " \
                       "DENY_PIXEL_SHADER_ROOT_ACCESS), " \
            "DescriptorTable(CBV(b0)," \
                            "SRV(t0)," \
                            "UAV(u0))"

Texture2DArray<unorm float4> srcTex;
RWTexture2DArray<unorm float4> dstTex;

[numthreads(SAME_RESOURCE_X, SAME_RESOURCE_Y, SAME_RESOURCE_Z)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    if (targetMip == 0) {
        dstTex[DTI] = float4(0, 0, 0, 1);
    } else {
        uint w, h, e;

        dstTex.GetDimensions(w, h, e);

        if (all(DTI.xy < uint2(w, h))) {
            dstTex[DTI] = float4(srcTex[DTI].rgb + 0.1f, 1);
        }
    }
}