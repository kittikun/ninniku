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

float max3(float3 value)
{
    return max(max(value.x, value.y), value.z);
}

// Get the uv to fetch a cubemap as Texture2DArray
// https://github.com/TheRealMJP/MSAAFilter/blob/master/SampleFramework11/v1.01/Graphics/Textures.h
float3 CubemapDirToTexture2DArray(float3 dir)
{
    float maxComponent = max3(abs(dir));

    uint face = 0;
    float2 uv = dir.yz;

    if (dir.x == maxComponent) {
        face = 0;
        uv = float2(-dir.z, -dir.y) / dir.x;
    } else if (-dir.x == maxComponent) {
        face = 1;
        uv = float2(dir.z, -dir.y) / -dir.x;
    } else if (dir.y == maxComponent) {
        face = 2;
        uv = float2(dir.x, dir.z) / dir.y;
    } else if (-dir.y == maxComponent) {
        face = 3;
        uv = float2(dir.x, -dir.z) / -dir.y;
    } else if (dir.z == maxComponent) {
        face = 4;
        uv = float2(dir.x, -dir.y) / dir.z;
    } else if (-dir.z == maxComponent) {
        face = 5;
        uv = float2(-dir.x, -dir.y) / -dir.z;
    }

    const float2 centerUV = float2(0.5f, 0.5f);

    uv = mad(uv, centerUV, centerUV);

    return float3(uv, face);
}