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

#include "../cbuffers.h"
#include "../color20.hlsl"

#define RS  "RootFlags( DENY_VERTEX_SHADER_ROOT_ACCESS | " \
                       "DENY_HULL_SHADER_ROOT_ACCESS | " \
                       "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                       "DENY_GEOMETRY_SHADER_ROOT_ACCESS | " \
                       "DENY_PIXEL_SHADER_ROOT_ACCESS), " \
            "DescriptorTable( UAV(u0))"

RWTexture2DArray<float4> dstTex;

[numthreads(COLORFACES_NUMTHREAD_X, COLORFACES_NUMTHREAD_Y, COLORFACES_NUMTHREAD_Z)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    // we can skip bound checks because we use a fixed size of 512
    // If you use a numthread that is not a power of 2, you might want to add one
    dstTex[DTI] = float4(color20[DTI.z], 1);
}