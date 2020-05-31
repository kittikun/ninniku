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

// https://www.khronos.org/registry/OpenGL/specs/gl/glspec42.core.pdf
// p244, it's opengl but should be the same for DX
static const float3 s_faceUvVectors[6][3] = {
    {
        // +x face
        {  0.0f,  0.0f, -1.0f }, // u -> -z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  1.0f,  0.0f,  0.0f }, // +x face
    },
    {
        // -x face
        {  0.0f,  0.0f,  1.0f }, // u -> +z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        { -1.0f,  0.0f,  0.0f }, // -x face
    },
    {
        // +y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f,  1.0f }, // v -> +z
        {  0.0f,  1.0f,  0.0f }, // +y face
    },
    {
        // -y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f, -1.0f }, // v -> -z
        {  0.0f, -1.0f,  0.0f }, // -y face
    },
    {
        // +z face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f,  1.0f }, // +z face
    },
    {
        // -z face
        { -1.0f,  0.0f,  0.0f }, // u -> -x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f, -1.0f }, // -z face
    }
};

float max3(float3 value)
{
    return max(max(value.x, value.y), value.z);
}

float2 packNormal(float3 n)
{
    n = normalize(n);
    return n.xy * rcp(n.z);
}

float pow3(float x)
{
    return x * x * x;
}

float3 unpackNormal(float2 packed)
{
    return normalize(float3(packed, 1));
}

float3 uvToVec(float3 uv)
{
    float2 viewPos = mad(uv.xy, 2.0, -1.0);

    return normalize(viewPos.x * s_faceUvVectors[uv.z][0] + viewPos.y * s_faceUvVectors[uv.z][1] + s_faceUvVectors[uv.z][2]);
}

float3 vecToUv(float3 dir)
{
    float3 res;
    float3 absVec = abs(dir);
    float max = max3(absVec);

    // Get face id (max component == face vector).
    if (max == absVec.x) {
        res.z = (dir.x >= 0.0f) ? 0 : 1;
    } else if (max == absVec.y) {
        res.z = (dir.y >= 0.0f) ? 2 : 3;
    } else { //if (max == absVec.z)
        res.z = (dir.z >= 0.0f) ? 4 : 5;
    }

    // normalize
    float3 faceVec = dir * rcp(max);

    res.x = (dot(s_faceUvVectors[res.z][0], faceVec) + 1.0f) * 0.5f;
    res.y = (dot(s_faceUvVectors[res.z][1], faceVec) + 1.0f) * 0.5f;

    return res;
}