RWTexture2D<float3> dstTex;

[numthreads(32, 32, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
    uint w, h;

    dstTex.GetDimensions(w, h);

    float r = float(w - DTI.x) * rcp(w);
    float g = float(h - DTI.y) * rcp(h);
    float b = float(DTI.x) * rcp(w);

    dstTex[DTI.xy] = float3(r, g, b);
}