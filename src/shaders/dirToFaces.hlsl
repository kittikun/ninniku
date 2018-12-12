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
#include "utility.hlsl"

TextureCube srcTex;
RWTexture2DArray<float4> dstTex;
SamplerState ssPoint;

[numthreads(DIRTOFACE_NUMTHREAD_X, DIRTOFACE_NUMTHREAD_Y, DIRTOFACE_NUMTHREAD_Z)]
void main(int3 DTI : SV_DispatchThreadID)
{
    float w, dummy1, dummy2;

    dstTex.GetDimensions(w, dummy1, dummy2);

    float2 uv = float2(DTI.xy) * rcp(w);
    float2 viewPos = mad(float2(uv.x, 1.0 - uv.y), 2.0, -1.0);
    //float2 viewPos = (float2)0;
    float4 dir = mul(float4(viewPos, 0, 1), invProjMat);
    dir = dir / dir.w;
    dir = mul(dir, invViewMat);
    float3 uv3 = CubemapDirToTexture2DArray(normalize(dir.xyz));
    uint3 pos = uint3(uv3.xy * w, uv3.z);
    //uint3 pos = uint3(DTI.xy, uv3.z);

    dstTex[pos] = srcTex.SampleLevel(ssPoint, normalize(dir.xyz), 0);
}