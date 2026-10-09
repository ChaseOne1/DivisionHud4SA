#include "render/BlurBackground.hpp"
#include "core/Config.hpp"
#include "render/Pipeline.hpp"
#include "render/ResourceText.hpp"
#include "render/Shader.hpp"
#include "render/Surface.hpp"

#include <algorithm>

namespace division::render {
namespace {
    char const* BLUR_SHADER_SOURCE = "sampler2D tex0 : register(s0);\n"
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
    if (m_pixel_shader)
        m_pixel_shader->Release();
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
    m_screen_width = 0;
    m_screen_height = 0;
}

bool BlurBackground::EnsureTargets(IDirect3DDevice9* device, int width, int height, D3DFORMAT format)
{
    if (m_screen_width == width && m_screen_height == height && m_half.texture) return true;

    ReleaseTargets();

    int const half_w = std::max(1, width / 2);
    int const half_h = std::max(1, height / 2);
    int const quarter_w = std::max(1, width / 4);
    int const quarter_h = std::max(1, height / 4);

    struct Spec
    {
        Target* target;
        int w;
        int h;
    };
    Spec const specs[] = {
        { &m_half, half_w, half_h },
        { &m_quarter, quarter_w, quarter_h },
        { &m_temp, quarter_w, quarter_h },
        { &m_result, quarter_w, quarter_h },
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

    m_screen_width = width;
    m_screen_height = height;
    return true;
}

bool BlurBackground::EnsureShader(IDirect3DDevice9* device)
{
    if (m_pixel_shader) return true;

    static std::string const source = LoadResourceText("DivisionHud4SA/shaders/blur.hlsl", BLUR_SHADER_SOURCE);
    m_pixel_shader = CompilePixelShader(device, source.c_str());
    return m_pixel_shader != nullptr;
}

void BlurBackground::BlurPass(IDirect3DDevice9* device, Target& destination, IDirect3DTexture9* source,
    float step_x, float step_y)
{
    DeviceStateGuard guard(device);

    device->SetRenderTarget(0, destination.surface);

    D3DVIEWPORT9 viewport { };
    viewport.Width = static_cast<DWORD>(destination.width);
    viewport.Height = static_cast<DWORD>(destination.height);
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    device->SetViewport(&viewport);

    device->SetVertexShader(nullptr);
    device->SetPixelShader(m_pixel_shader);
    device->SetFVF(VERTEX_FVF);
    device->SetTexture(0, source);

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    // The scene's post-effects can leave the stencil test / colour-write mask enabled;
    // force them off so the blur passes are never masked out.
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    device->SetRenderState(D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    float const params[4] = { step_x, step_y, core::cfg.BlurSaturation, 0.0f };
    device->SetPixelShaderConstantF(0, params, 1);

    float const w = static_cast<float>(destination.width);
    float const h = static_cast<float>(destination.height);
    Vertex const vertices[4] = {
        { 0.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 0.0f },
        { w, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 0.0f },
        { 0.0f, h, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 1.0f },
        { w, h, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 1.0f },
    };
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
}

IDirect3DTexture9* BlurBackground::Update(IDirect3DDevice9* device)
{
    IDirect3DSurface9* back_buffer = nullptr;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer)) || !back_buffer) return nullptr;

    D3DSURFACE_DESC desc { };
    back_buffer->GetDesc(&desc);

    if (!EnsureTargets(device, static_cast<int>(desc.Width), static_cast<int>(desc.Height), desc.Format) || !EnsureShader(device)) {
        back_buffer->Release();
        return nullptr;
    }

    device->StretchRect(back_buffer, nullptr, m_half.surface, nullptr, D3DTEXF_LINEAR);
    device->StretchRect(m_half.surface, nullptr, m_quarter.surface, nullptr, D3DTEXF_LINEAR);

    float const step_x = core::cfg.BlurSpacing / static_cast<float>(m_quarter.width);
    float const step_y = core::cfg.BlurSpacing / static_cast<float>(m_quarter.height);

    IDirect3DTexture9* input = m_quarter.texture;
    for (int i = 0; i < core::cfg.BlurIterations; ++i) {
        BlurPass(device, m_temp, input, step_x, 0.0f);
        BlurPass(device, m_result, m_temp.texture, 0.0f, step_y);
        input = m_result.texture;
    }

    back_buffer->Release();
    return m_result.texture;
}

void BlurBackground::OnDeviceLost()
{
    ReleaseTargets();
    if (m_pixel_shader) {
        m_pixel_shader->Release();
        m_pixel_shader = nullptr;
    }
}
}
