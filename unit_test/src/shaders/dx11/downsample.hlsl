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

// Use point for since we're going to average samples anyway
SamplerState ssPoint;
Texture2DArray<float4> srcMip;
RWTexture2DArray<float4> dstMipSlice;

[numthreads(DOWNSAMPLE_NUMTHREAD_X, DOWNSAMPLE_NUMTHREAD_Y, DOWNSAMPLE_NUMTHREAD_Z)]
void main(uint3 GID : SV_GroupID, uint3 GTI : SV_GroupThreadID)
{
    uint size, dummy1, dummy2;

    // we always slice for one mip level so no need to specify target mip
    // Note that this version of GetDimensions works with WARP but not a proper GPU
    srcMip.GetDimensions(size, dummy1, dummy2);

    uint2 samplePos = mad(GID.xy, DOWNSAMPLE_NUMTHREAD_X << 1, GTI.xy << 1); // on srcMip, x2 because we sample 2x2 with gather
    float3 startPosUV = float3(rcp((float2)size) * float2(samplePos + (float2)0.5), GID.z);

    // srcMip -> dstMipSlice0
    if (all(samplePos < (uint2)size)) {
        float3 res;
        float4 tmp;

        tmp = srcMip.GatherRed(ssPoint, startPosUV, (int2)0);
        res.r = tmp.x + tmp.y + tmp.z + tmp.w;
        tmp = srcMip.GatherGreen(ssPoint, startPosUV, (int2)0);
        res.g = tmp.x + tmp.y + tmp.z + tmp.w;
        tmp = srcMip.GatherBlue(ssPoint, startPosUV, (int2)0);
        res.b = tmp.x + tmp.y + tmp.z + tmp.w;
        res *= rcp(4.0);

        dstMipSlice[uint3(mad(GID.xy, DOWNSAMPLE_NUMTHREAD_X, GTI.xy), GID.z)] = float4(res, 1);
    }
}