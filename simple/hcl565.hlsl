#include "common.hlsl"

Texture2D<float4> srcTex;
RWTexture2D<float4> dstTex;

float HCLgamma = 3;
float HCLy0 = 100;
float HCLmaxL = 0.530454533953517; // == exp(HCLgamma / HCLy0) - 0.5
float PI = 3.1415926536;

float3 HCLtoRGB(in float3 HCL)
{
    float3 RGB = 0;
    if (HCL.z != 0) {
        float H = HCL.x;
        float C = HCL.y;
        float L = HCL.z * HCLmaxL;
        float Q = exp((1 - C / (2 * L)) * (HCLgamma / HCLy0));
        float U = (2 * L - C) / (2 * Q - 1);
        float V = C / Q;
        float A = (H + min(frac(2 * H) / 4, frac(-2 * H) / 8)) * PI * 2;
        float T;
        H *= 6;
        if (H <= 0.999) {
            T = tan(A);
            RGB.r = 1;
            RGB.g = T / (1 + T);
        } else if (H <= 1.001) {
            RGB.r = 1;
            RGB.g = 1;
        } else if (H <= 2) {
            T = tan(A);
            RGB.r = (1 + T) / T;
            RGB.g = 1;
        } else if (H <= 3) {
            T = tan(A);
            RGB.g = 1;
            RGB.b = 1 + T;
        } else if (H <= 3.999) {
            T = tan(A);
            RGB.g = 1 / (1 + T);
            RGB.b = 1;
        } else if (H <= 4.001) {
            RGB.g = 0;
            RGB.b = 1;
        } else if (H <= 5) {
            T = tan(A);
            RGB.r = -1 / T;
            RGB.b = 1;
        } else {
            T = tan(A);
            RGB.r = 1;
            RGB.b = -T;
        }
        RGB = RGB * V + U;
    }
    return RGB;
}

float3 RGBtoHCL(in float3 RGB)
{
    float3 HCL;
    float H = 0;
    float U = min(RGB.r, min(RGB.g, RGB.b));
    float V = max(RGB.r, max(RGB.g, RGB.b));
    float Q = HCLgamma / HCLy0;
    HCL.y = V - U;
    if (HCL.y != 0) {
        H = atan2(RGB.g - RGB.b, RGB.r - RGB.g) / PI;
        Q *= U / V;
    }
    Q = exp(Q);
    HCL.x = frac(H / 2 - min(frac(H), frac(-H)) / 6);
    HCL.y *= Q;
    HCL.z = lerp(-U, V, Q) / (HCLmaxL * 2);
    return HCL;
}

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = RGBtoHCL(srcTex[DTI.xy].rgb);

    float3 temp = To565nFrom(src);

    dstTex[DTI.xy] = float4(HCLtoRGB(temp), 1);
}