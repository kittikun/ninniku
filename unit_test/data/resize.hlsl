// Copyright(c) 2018-2019 Kitti Vongsay
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

#include "cbuffers.h"

Texture2DArray<float4> srcTex;
RWTexture2DArray<float4> dstTex;
SamplerState ssLinear;

[numthreads(RESIZE_NUMTHREAD_X, RESIZE_NUMTHREAD_X, RESIZE_NUMTHREAD_Z)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float w, dummy1, dummy2;

    dstTex.GetDimensions(w, dummy1, dummy2);

    float2 uv = (float2)rcp(w) * (float2(DTI.xy) + (float2)0.5);

    dstTex[DTI] = srcTex.SampleLevel(ssLinear, float3(uv, DTI.z), 0);
}