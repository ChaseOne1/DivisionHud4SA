float4 main(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR
{
    float d = abs(uv.x);
    float a = saturate(uv.y + 0.5 - d);
    return float4(color.rgb, color.a * a);
}
