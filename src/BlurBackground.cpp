#include "BlurBackground.h"
#include "Config.h"

#include <algorithm>
#include <cstring>
#include <d3dcompiler.h>

namespace division {
namespace {
    constexpr DWORD kFullscreenFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

    struct FullscreenVertex
    {
        float x, y, z, rhw;
        D3DCOLOR color;
        float u, v;
    };

    char const* kBlurShaderSource = "sampler2D tex0 : register(s0);\n"
                                    "float4 params : register(c0);\n"
                                    "float4 main(float2 uv : TEXCOORD0) : COLOR\n"
                                    "{\n"
                                    "    float2 step = params.xy;\n"
                                    "    float saturation = params.z;\n"
                                    "    float4 c  = tex2D(tex0, uv) * 0.18;\n"
                                    "    c += tex2D(tex0, uv + step) * 0.15;\n"
                                    "    c += tex2D(tex0, uv - step) * 0.15;\n"
                                    "    c += tex2D(tex0, uv + step * 2.0) * 0.12;\n"
                                    "    c += tex2D(tex0, uv - step * 2.0) * 0.12;\n"
                                    "    c += tex2D(tex0, uv + step * 3.0) * 0.09;\n"
                                    "    c += tex2D(tex0, uv - step * 3.0) * 0.09;\n"
                                    "    c += tex2D(tex0, uv + step * 4.0) * 0.05;\n"
                                    "    c += tex2D(tex0, uv - step * 4.0) * 0.05;\n"
                                    "    float lum = dot(c.rgb, float3(0.299, 0.587, 0.114));\n"
                                    "    float3 gray = lerp(float3(lum, lum, lum), c.rgb, saturation);\n"
                                    "    return float4(gray, 1.0);\n"
                                    "}\n";
}

BlurBackground::~BlurBackground()
{
    ReleaseTargets();
    if (m_pixelShader)
        m_pixelShader->Release();
}

void BlurBackground::ReleaseTarget(Target& target)
{
    if (target.surface) {
        target.surface->Release();
        target.surface = nullptr;
    }
    if (target.texture) {
        target.texture->Release();
        target.texture = nullptr;
    }
    target.width = 0;
    target.height = 0;
}

void BlurBackground::ReleaseTargets()
{
    ReleaseTarget(m_half);
    ReleaseTarget(m_quarter);
    ReleaseTarget(m_temp);
    ReleaseTarget(m_result);
    m_screenWidth = 0;
    m_screenHeight = 0;
}

bool BlurBackground::EnsureTargets(IDirect3DDevice9* device, int width, int height, D3DFORMAT format)
{
    if (m_screenWidth == width && m_screenHeight == height && m_half.texture)
        return true;

    ReleaseTargets();

    int const halfW = std::max(1, width / 2);
    int const halfH = std::max(1, height / 2);
    int const quarterW = std::max(1, width / 4);
    int const quarterH = std::max(1, height / 4);

    struct Spec
    {
        Target* target;
        int w;
        int h;
    };
    Spec const specs[] = {
        { &m_half, halfW, halfH },
        { &m_quarter, quarterW, quarterH },
        { &m_temp, quarterW, quarterH },
        { &m_result, quarterW, quarterH },
    };

    for (auto const& spec : specs) {
        if (FAILED(device->CreateTexture(spec.w, spec.h, 1, D3DUSAGE_RENDERTARGET, format,
                D3DPOOL_DEFAULT, &spec.target->texture, nullptr))) {
            ReleaseTargets();
            return false;
        }
        if (FAILED(spec.target->texture->GetSurfaceLevel(0, &spec.target->surface))) {
            ReleaseTargets();
            return false;
        }
        spec.target->width = spec.w;
        spec.target->height = spec.h;
    }

    m_screenWidth = width;
    m_screenHeight = height;
    return true;
}

bool BlurBackground::EnsureShader(IDirect3DDevice9* device)
{
    if (m_pixelShader)
        return true;

    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT const hr = D3DCompile(kBlurShaderSource, std::strlen(kBlurShaderSource),
        nullptr, nullptr, nullptr, "main", "ps_2_0",
        0, 0, &code, &errors);
    if (errors)
        errors->Release();
    if (FAILED(hr) || !code)
        return false;

    HRESULT const create = device->CreatePixelShader(reinterpret_cast<DWORD const*>(code->GetBufferPointer()),
        &m_pixelShader);
    code->Release();
    return SUCCEEDED(create);
}

void BlurBackground::BlurPass(IDirect3DDevice9* device, Target& destination, IDirect3DTexture9* source,
    float stepX, float stepY)
{
    IDirect3DSurface9* previousTarget = nullptr;
    device->GetRenderTarget(0, &previousTarget);

    D3DVIEWPORT9 previousViewport { };
    device->GetViewport(&previousViewport);

    IDirect3DBaseTexture9* previousTexture = nullptr;
    device->GetTexture(0, &previousTexture);

    IDirect3DPixelShader9* previousShader = nullptr;
    device->GetPixelShader(&previousShader);

    device->SetRenderTarget(0, destination.surface);

    D3DVIEWPORT9 viewport { };
    viewport.Width = static_cast<DWORD>(destination.width);
    viewport.Height = static_cast<DWORD>(destination.height);
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    device->SetViewport(&viewport);

    device->SetVertexShader(nullptr);
    device->SetPixelShader(m_pixelShader);
    device->SetFVF(kFullscreenFvf);
    device->SetTexture(0, source);

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    float const params[4] = { stepX, stepY, Config::BlurSaturation, 0.0f };
    device->SetPixelShaderConstantF(0, params, 1);

    float const w = static_cast<float>(destination.width);
    float const h = static_cast<float>(destination.height);
    FullscreenVertex const vertices[4] = {
        { 0.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 0.0f },
        { w, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 0.0f },
        { 0.0f, h, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 1.0f },
        { w, h, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 1.0f },
    };
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(FullscreenVertex));

    device->SetRenderTarget(0, previousTarget);
    device->SetViewport(&previousViewport);
    device->SetTexture(0, previousTexture);
    device->SetPixelShader(previousShader);

    if (previousTarget) previousTarget->Release();
    if (previousTexture) previousTexture->Release();
    if (previousShader) previousShader->Release();
}

IDirect3DTexture9* BlurBackground::Update(IDirect3DDevice9* device)
{
    IDirect3DSurface9* backBuffer = nullptr;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
        return nullptr;

    D3DSURFACE_DESC desc { };
    backBuffer->GetDesc(&desc);

    if (!EnsureTargets(device, static_cast<int>(desc.Width), static_cast<int>(desc.Height), desc.Format) || !EnsureShader(device)) {
        backBuffer->Release();
        return nullptr;
    }

    device->StretchRect(backBuffer, nullptr, m_half.surface, nullptr, D3DTEXF_LINEAR);
    device->StretchRect(m_half.surface, nullptr, m_quarter.surface, nullptr, D3DTEXF_LINEAR);

    float const stepX = Config::BlurSpacing / static_cast<float>(m_quarter.width);
    float const stepY = Config::BlurSpacing / static_cast<float>(m_quarter.height);

    IDirect3DTexture9* input = m_quarter.texture;
    for (int i = 0; i < Config::BlurIterations; ++i) {
        BlurPass(device, m_temp, input, stepX, 0.0f);
        BlurPass(device, m_result, m_temp.texture, 0.0f, stepY);
        input = m_result.texture;
    }

    backBuffer->Release();
    return m_result.texture;
}

void BlurBackground::OnDeviceLost()
{
    ReleaseTargets();
    if (m_pixelShader) {
        m_pixelShader->Release();
        m_pixelShader = nullptr;
    }
}
}
