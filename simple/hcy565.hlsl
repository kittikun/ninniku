#include "common.hlsl"

Texture2D<float3> srcTex;
RWTexture2D<float4> dstTex;

// http://www.chilliant.com/rgb2hsv.html
// The weights of RGB contributions to luminance.
// Should sum to unity.
float3 HCYwts = float3(0.299, 0.587, 0.114);

float3 HCYtoRGB(in float3 HCY)
{
    float3 RGB = HUEtoRGB(HCY.x);
    float Z = dot(RGB, HCYwts);
    if (HCY.z < Z) {
        HCY.y *= HCY.z / Z;
    } else if (Z < 1) {
        HCY.y *= (1 - HCY.z) / (1 - Z);
    }
    return (RGB - Z) * HCY.y + HCY.z;
}

float3 RGBtoHCY(in float3 RGB)
{
    // Corrected by David Schaeffer
    float3 HCV = RGBtoHCV(RGB);
    float Y = dot(RGB, HCYwts);
    float Z = dot(HUEtoRGB(HCV.x), HCYwts);
    if (Y < Z) {
        HCV.y *= Z / (Epsilon + Y);
    } else {
        HCV.y *= (1 - Z) / (Epsilon + 1 - Y);
    }
    return float3(HCV.x, HCV.y, Y);
}

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = RGBtoHCY(srcTex[DTI.xy]);
    float3 temp = To667nFrom(src);

    dstTex[DTI.xy] = float4(HCYtoRGB(temp), 1);
}