#include "common.hlsl"

Texture2D<float3> srcTex;
RWTexture2D<float4> dstTex;

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    float3 src = srcTex[DTI.xy];

    dstTex[DTI.xy] = float4(To676nFrom(src), 1);
}