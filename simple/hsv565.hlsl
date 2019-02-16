#include "common.hlsl"

Texture2D<float3> srcTex;
RWTexture2D<float4> dstTex;

// http://www.chilliant.com/rgb2hsv.html

float3 RGBtoHSV(in float3 RGB)
{
    float3 HCV = RGBtoHCV(RGB);
    float S = HCV.y / (HCV.z + Epsilon);
    return float3(HCV.x, S, HCV.z);
}

float3 HSVtoRGB(in float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = RGBtoHSV(srcTex[DTI.xy]);
    float3 temp = To766nFrom(src);

    dstTex[DTI.xy] = float4(HSVtoRGB(temp), 1);
}