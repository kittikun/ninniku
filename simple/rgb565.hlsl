#include "common.hlsl"

Texture2D<float4> srcTex;
RWTexture2D<float4> dstTex;

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = srcTex[DTI.xy].rgb;

    dstTex[DTI.xy] = float4(To565nFrom(src), 1);
}