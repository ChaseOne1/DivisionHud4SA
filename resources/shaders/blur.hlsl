sampler2D tex0 : register(s0);
float4 params : register(c0);
float4 main(float2 uv : TEXCOORD0) : COLOR
{
    float2 step = params.xy;
    float saturation = params.z;
    float4 c  = tex2D(tex0, uv) * 0.18;
    c += tex2D(tex0, uv + step) * 0.15;
    c += tex2D(tex0, uv - step) * 0.15;
    c += tex2D(tex0, uv + step * 2.0) * 0.12;
    c += tex2D(tex0, uv - step * 2.0) * 0.12;
    c += tex2D(tex0, uv + step * 3.0) * 0.09;
    c += tex2D(tex0, uv - step * 3.0) * 0.09;
    c += tex2D(tex0, uv + step * 4.0) * 0.05;
    c += tex2D(tex0, uv - step * 4.0) * 0.05;
    float lum = dot(c.rgb, float3(0.299, 0.587, 0.114));
    float3 gray = lerp(float3(lum, lum, lum), c.rgb, saturation);
    return float4(gray, 1.0);
}
