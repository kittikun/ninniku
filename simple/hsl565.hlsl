#include "common.hlsl"

Texture2D<float3> srcTex;
RWTexture2D<float4> dstTex;

// http://www.chilliant.com/rgb2hsv.html

float3 HSLtoRGB(in float3 HSL)
{
    float3 RGB = HUEtoRGB(HSL.x);
    float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
    return (RGB - 0.5) * C + HSL.z;
}

float3 RGBtoHSL(in float3 RGB)
{
    float3 HCV = RGBtoHCV(RGB);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
    return float3(HCV.x, S, L);
}

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = RGBtoHSL(srcTex[DTI.xy]);
    float3 temp = To766nFrom(src);

    dstTex[DTI.xy] = float4(HSLtoRGB(temp), 1);
}