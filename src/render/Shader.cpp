#include "render/Shader.hpp"

#include "render/ResourceText.hpp"

#include <cstring>
#include <d3dcompiler.h>

namespace division::render {

IDirect3DPixelShader9* CompilePixelShader(IDirect3DDevice9* device, char const* source)
{
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT const hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "main", "ps_2_0", 0, 0,
        &code, &errors);
    if (errors)
        errors->Release();
    if (FAILED(hr) || !code) return nullptr;

    IDirect3DPixelShader9* shader = nullptr;
    device->CreatePixelShader(reinterpret_cast<DWORD const*>(code->GetBufferPointer()), &shader);
    code->Release();
    return shader;
}

IDirect3DPixelShader9* CreateLineShader(IDirect3DDevice9* device)
{
    static std::string const source = LoadResourceText("DivisionHud4SA/shaders/line.hlsl",
        "float4 main(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR\n"
        "{\n"
        "    float d = abs(uv.x);\n"
        "    float a = saturate(uv.y + 0.5 - d);\n"
        "    return float4(color.rgb, color.a * a);\n"
        "}\n");
    return CompilePixelShader(device, source.c_str());
}

IDirect3DPixelShader9* CreateBarShader(IDirect3DDevice9* device)
{
    static std::string const source = LoadResourceText("DivisionHud4SA/shaders/bar.hlsl",
        "float4 main(float2 a : TEXCOORD0, float2 b : TEXCOORD1, float4 color : COLOR0) : COLOR\n"
        "{\n"
        "    float d = min(min(a.x, a.y), min(b.x, b.y));\n"
        "    float cov = saturate(d + 0.5);\n"
        "    return float4(color.rgb, color.a * cov);\n"
        "}\n");
    return CompilePixelShader(device, source.c_str());
}

IDirect3DPixelShader9* CreateWaveShader(IDirect3DDevice9* device)
{
    static std::string const source = LoadResourceText("DivisionHud4SA/shaders/wave.hlsl",
        "float4 params : register(c0);\n"
        "float4 main(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR\n"
        "{\n"
        "    float g = 1.0 - smoothstep(params.x, params.x + max(params.y, 0.0001), uv.x);\n"
        "    float dy = min(uv.y, 1.0 - uv.y) * params.z;\n"
        "    float dx = uv.x * params.w;\n"
        "    g *= saturate(dy + 0.5) * saturate(dx + 0.5);\n"
        "    return float4(color.rgb, color.a * g);\n"
        "}\n");
    return CompilePixelShader(device, source.c_str());
}

}