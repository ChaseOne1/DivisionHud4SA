float4 main(float2 a : TEXCOORD0, float2 b : TEXCOORD1, float4 color : COLOR0) : COLOR
{
    float d = min(min(a.x, a.y), min(b.x, b.y));
    float cov = saturate(d + 0.5);
    return float4(color.rgb, color.a * cov);
}
