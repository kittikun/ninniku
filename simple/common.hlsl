#ifndef COMMON_HLSL
#define COMMON_HLSL

// http://www.chilliant.com/rgb2hsv.html

float Epsilon = 1e-10;

float3 RGBtoHCV(in float3 RGB)
{
    // Based on work by Sam Hocevar and Emil Persson
    float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0 / 3.0) : float4(RGB.gb, 0.0, -1.0 / 3.0);
    float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return float3(H, C, Q.x);
}

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 Convert(float3 val, uint3 range)
{
    const float3 frange = float3(range);
    uint3 temp = uint3(floor(val * frange));

    temp = uint3(temp.r & range.r, temp.g & range.g, temp.b & range.b);

    return float3(temp) / frange;
}

float3 To655nFrom(float3 val)
{
    const uint3 range = uint3(0x3f, 0x1f, 0x1f);

    return Convert(val, range);
}

float3 To565nFrom(float3 val)
{
    const uint3 range = uint3(0x1f, 0x3f, 0x1f);

    return Convert(val, range);
}

float3 To556nFrom(float3 val)
{
    const uint3 range = uint3(0x1f, 0x1f, 0x3f);

    return Convert(val, range);
}

float3 To766nFrom(float3 val)
{
    const uint3 range = uint3(0x7f, 0x3f, 0x3f);

    return Convert(val, range);
}

float3 To676nFrom(float3 val)
{
    const uint3 range = uint3(0x3f, 0x7f, 0x3f);

    return Convert(val, range);
}

float3 To667nFrom(float3 val)
{
    const uint3 range = uint3(0x3f, 0x3f, 0x7f);

    return Convert(val, range);
}

#endif // COMMON_HLSL