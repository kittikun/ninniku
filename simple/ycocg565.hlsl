#include "common.hlsl"

Texture2D<float3> srcTex;
RWTexture2D<float4> dstTex;

float3 YCoCg_FromRGB(in float3 color)
{
    return float3(0.25f * color.r + 0.5f * color.g + 0.25f * color.b,
                  0.5f * color.r - 0.5f * color.b + 0.5f,
                  -0.25f * color.r + 0.5f * color.g - 0.25f * color.b + 0.5f);
}

float3 YCoCg_ToRGB(in float3 ycocg)
{
    ycocg.y -= 0.5f;
    ycocg.z -= 0.5f;
    return float3(ycocg.r + ycocg.g - ycocg.b,
                  ycocg.r + ycocg.b,
                  ycocg.r - ycocg.g - ycocg.b);
}

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = YCoCg_FromRGB(srcTex[DTI.xy]);
    float3 temp = To766nFrom(src);

    dstTex[DTI.xy] = float4(YCoCg_ToRGB(temp), 1);
}