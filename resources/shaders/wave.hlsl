// Wanted-bar sweep: a rectangle that grows from the bar start up to the sweep head.
// Feathers the leading edge (params.x/params.y), the top/bottom (params.z) and the
// left edge (params.w) so the rectangle has no jaggies.
// uv.x = position along the bar (0 at the fill start), uv.y = 0..1 across the height.
float4 params : register(c0);

float4 main(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR
{
    float g = 1.0 - smoothstep(params.x, params.x + max(params.y, 0.0001), uv.x);
    float dy = min(uv.y, 1.0 - uv.y) * params.z;
    float dx = uv.x * params.w;
    g *= saturate(dy + 0.5) * saturate(dx + 0.5);
    return float4(color.rgb, color.a * g);
}
