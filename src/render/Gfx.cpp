#include "render/Gfx.hpp"

#include "plugin.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <d3dcompiler.h>

namespace division::render {
namespace {
    float const PI_F = static_cast<float>(PI);
    constexpr float AASOFTNESS = 1.0f;
    constexpr float SOLID_COVERAGE = 64.0f;

    void FillVertex(Vertex& v, ScreenPoint const& p, D3DCOLOR color)
    {
        v.x = p.x;
        v.y = p.y;
        v.z = 0.0f;
        v.rhw = 1.0f;
        v.color = color;
        v.u = 0.0f;
        v.v = 0.0f;
    }

    float ScreenDistance(ScreenPoint const& a, ScreenPoint const& b)
    {
        float const dx = a.x - b.x;
        float const dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    void AppendSoftRect(std::vector<BarVertex>& out, ScreenPoint const& tl, ScreenPoint const& tr,
        ScreenPoint const& bl, ScreenPoint const& br, D3DCOLOR color)
    {
        float lx = tr.x - tl.x;
        float ly = tr.y - tl.y;
        float const ll = std::sqrt(lx * lx + ly * ly);
        if (ll < 0.001f) return;
        lx /= ll;
        ly /= ll;

        float px = bl.x - tl.x;
        float py = bl.y - tl.y;
        float const pl = std::sqrt(px * px + py * py);
        if (pl < 0.001f) return;
        px /= pl;
        py /= pl;

        float const width = (ll + ScreenDistance(bl, br)) * 0.5f;
        float const height = (pl + ScreenDistance(tr, br)) * 0.5f;
        float const pad = AASOFTNESS;

        ScreenPoint const otl = { tl.x - px * pad - lx * pad, tl.y - py * pad - ly * pad };
        ScreenPoint const otr = { tr.x - px * pad + lx * pad, tr.y - py * pad + ly * pad };
        ScreenPoint const obl = { bl.x + px * pad - lx * pad, bl.y + py * pad - ly * pad };
        ScreenPoint const obr = { br.x + px * pad + lx * pad, br.y + py * pad + ly * pad };

        float const inner = -pad;
        float const outer_x = width + pad;
        float const outer_y = height + pad;

        BarVertex v[4];
        auto set = [&](BarVertex& vert, ScreenPoint const& p, float dl, float dr, float dt, float db) {
            vert.x = p.x;
            vert.y = p.y;
            vert.z = 0.0f;
            vert.rhw = 1.0f;
            vert.color = color;
            vert.u = dl;
            vert.v = dr;
            vert.u2 = dt;
            vert.v2 = db;
        };
        set(v[0], otl, inner, outer_x, inner, outer_y);
        set(v[1], otr, outer_x, inner, inner, outer_y);
        set(v[2], obl, inner, outer_x, outer_y, inner);
        set(v[3], obr, outer_x, inner, outer_y, inner);

        out.push_back(v[0]);
        out.push_back(v[1]);
        out.push_back(v[2]);
        out.push_back(v[2]);
        out.push_back(v[1]);
        out.push_back(v[3]);
    }
}

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

std::string LoadResourceText(char const* relative_path, char const* fallback)
{
    HANDLE const file = CreateFileA(PLUGIN_PATH(relative_path), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size { };
        if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < (1 << 20)) {
            std::string text(static_cast<size_t>(size.QuadPart), '\0');
            DWORD read = 0;
            if (ReadFile(file, text.data(), static_cast<DWORD>(text.size()), &read, nullptr) && read > 0) {
                CloseHandle(file);
                text.resize(read);
                return text;
            }
        }
        CloseHandle(file);
    }
    return fallback ? std::string(fallback) : std::string();
}

void PanelSurface::SetCorners(ScreenPoint const& top_left, ScreenPoint const& top_right,
    ScreenPoint const& bottom_right, ScreenPoint const& bottom_left)
{
    m_top_left = top_left;
    m_top_right = top_right;
    m_bottom_right = bottom_right;
    m_bottom_left = bottom_left;
}

ScreenPoint PanelSurface::Map(float nx, float ny) const
{
    float const bottom_x = m_bottom_left.x + (m_bottom_right.x - m_bottom_left.x) * nx;
    float const bottom_y = m_bottom_left.y + (m_bottom_right.y - m_bottom_left.y) * nx;
    float const top_x = m_top_left.x + (m_top_right.x - m_top_left.x) * nx;
    float const top_y = m_top_left.y + (m_top_right.y - m_top_left.y) * nx;
    ScreenPoint out;
    out.x = bottom_x + (top_x - bottom_x) * ny;
    out.y = bottom_y + (top_y - bottom_y) * ny;
    return out;
}

D3DCOLOR ScaleColor(D3DCOLOR color, float extra)
{
    float const a = ((color >> 24) & 0xff) / 255.0f * std::clamp(extra, 0.0f, 1.0f);
    return (color & 0x00ffffffu) | (static_cast<DWORD>(a * 255.0f + 0.5f) << 24);
}

void DrawTexturedQuad(IDirect3DDevice9* device, ScreenPoint const (&points)[4], float const (&uv)[4][2],
    D3DCOLOR color)
{
    Vertex vertices[4];
    for (int i = 0; i < 4; ++i) {
        vertices[i].x = points[i].x;
        vertices[i].y = points[i].y;
        vertices[i].z = 0.0f;
        vertices[i].rhw = 1.0f;
        vertices[i].color = color;
        vertices[i].u = uv[i][0];
        vertices[i].v = uv[i][1];
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
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

void DrawLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to, float thickness,
    D3DCOLOR color)
{
    float const dx = to.x - from.x;
    float const dy = to.y - from.y;
    float const length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.001f) return;

    float const half = thickness * 0.5f;
    float const padded = half + 1.0f;
    float const nx = -dy / length * padded;
    float const ny = dx / length * padded;

    Vertex const vertices[4] = {
        { from.x + nx, from.y + ny, 0.0f, 1.0f, color, padded, half },
        { to.x + nx, to.y + ny, 0.0f, 1.0f, color, padded, half },
        { from.x - nx, from.y - ny, 0.0f, 1.0f, color, -padded, half },
        { to.x - nx, to.y - ny, 0.0f, 1.0f, color, -padded, half },
    };
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
}

void DrawEdgeLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to,
    ScreenPoint const& interior, float thickness, D3DCOLOR color)
{
    float const dx = to.x - from.x;
    float const dy = to.y - from.y;
    float const length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.001f) return;

    float nx = -dy / length;
    float ny = dx / length;
    if (nx * (interior.x - from.x) + ny * (interior.y - from.y) < 0.0f) {
        nx = -nx;
        ny = -ny;
    }

    float const offset = thickness * 0.5f + 0.5f;
    ScreenPoint const shifted_from = { from.x + nx * offset, from.y + ny * offset };
    ScreenPoint const shifted_to = { to.x + nx * offset, to.y + ny * offset };
    DrawLine(device, shifted_from, shifted_to, thickness, color);
}

void AppendSolidTri(std::vector<BarVertex>& out, ScreenPoint const& a, ScreenPoint const& b,
    ScreenPoint const& c, D3DCOLOR color)
{
    ScreenPoint const points[3] = { a, b, c };
    for (int i = 0; i < 3; ++i) {
        BarVertex v;
        v.x = points[i].x;
        v.y = points[i].y;
        v.z = 0.0f;
        v.rhw = 1.0f;
        v.color = color;
        v.u = SOLID_COVERAGE;
        v.v = SOLID_COVERAGE;
        v.u2 = SOLID_COVERAGE;
        v.v2 = SOLID_COVERAGE;
        out.push_back(v);
    }
}

void AppendAATri(std::vector<BarVertex>& out, ScreenPoint const& a, ScreenPoint const& b, ScreenPoint const& c,
    D3DCOLOR color)
{
    auto distance_to_edge = [](ScreenPoint const& p, ScreenPoint const& q, ScreenPoint const& r) {
        float const ex = q.x - p.x;
        float const ey = q.y - p.y;
        float const length = std::sqrt(ex * ex + ey * ey);
        if (length < 1e-6f) return 0.0f;
        return std::fabs((-ey * (r.x - p.x) + ex * (r.y - p.y)) / length);
    };

    // At each vertex only the distance to the opposite edge is non-zero; that distance is linear in
    // the barycentric coordinates, so the shader feathers all three edges exactly.
    float const da = distance_to_edge(b, c, a);
    float const db = distance_to_edge(c, a, b);
    float const dc = distance_to_edge(a, b, c);

    auto push = [&](ScreenPoint const& p, float u, float v, float u2) {
        BarVertex vertex;
        vertex.x = p.x;
        vertex.y = p.y;
        vertex.z = 0.0f;
        vertex.rhw = 1.0f;
        vertex.color = color;
        vertex.u = u;
        vertex.v = v;
        vertex.u2 = u2;
        vertex.v2 = SOLID_COVERAGE;
        out.push_back(vertex);
    };
    push(a, da, 0.0f, 0.0f);
    push(b, 0.0f, db, 0.0f);
    push(c, 0.0f, 0.0f, dc);
}

void AppendAARect(std::vector<BarVertex>& out, PanelSurface const& surface, float x0, float y0, float x1,
    float y1, D3DCOLOR color)
{
    AppendSoftRect(out, surface.Map(x0, y1), surface.Map(x1, y1), surface.Map(x0, y0), surface.Map(x1, y0),
        color);
}

void AppendSolidRect(std::vector<BarVertex>& out, PanelSurface const& surface, float x0, float y0, float x1,
    float y1, D3DCOLOR color)
{
    ScreenPoint const tl = surface.Map(x0, y1);
    ScreenPoint const tr = surface.Map(x1, y1);
    ScreenPoint const bl = surface.Map(x0, y0);
    ScreenPoint const br = surface.Map(x1, y0);
    AppendSolidTri(out, tl, tr, bl, color);
    AppendSolidTri(out, bl, tr, br, color);
}

void AppendRing(std::vector<BarVertex>& out, PanelSurface const& surface, float cx, float cy, float rx,
    float ry, float inner_ratio, D3DCOLOR color)
{
    constexpr int SEGMENTS = 40;
    float const inner = std::clamp(inner_ratio, 0.0f, 0.99f);
    ScreenPoint const center = surface.Map(cx, cy);

    // Encode the two circular boundaries (u = distance to the inner edge, v = distance
    // to the outer edge) so both edges feather while the radial seams stay solid.
    auto push = [&](ScreenPoint const& p, float to_inner, float to_outer) {
        BarVertex vertex;
        vertex.x = p.x;
        vertex.y = p.y;
        vertex.z = 0.0f;
        vertex.rhw = 1.0f;
        vertex.color = color;
        vertex.u = to_inner;
        vertex.v = to_outer;
        vertex.u2 = 64.0f;
        vertex.v2 = 64.0f;
        out.push_back(vertex);
    };
    auto edge = [&](float angle, float ratio) {
        return surface.Map(cx + std::cos(angle) * rx * ratio, cy + std::sin(angle) * ry * ratio);
    };

    for (int i = 0; i < SEGMENTS; ++i) {
        float const a0 = i * 2.0f * PI_F / SEGMENTS;
        float const a1 = (i + 1) * 2.0f * PI_F / SEGMENTS;
        ScreenPoint const o0 = edge(a0, 1.0f), o1 = edge(a1, 1.0f);
        ScreenPoint const t0 = edge(a0, inner), t1 = edge(a1, inner);
        float const ri0 = ScreenDistance(center, t0), ri1 = ScreenDistance(center, t1);
        float const ro0 = ScreenDistance(center, o0), ro1 = ScreenDistance(center, o1);
        float const b0 = std::max(0.0f, ro0 - ri0);
        float const b1 = std::max(0.0f, ro1 - ri1);
        push(t0, 0.0f, b0);
        push(o0, b0, 0.0f);
        push(t1, 0.0f, b1);
        push(t1, 0.0f, b1);
        push(o0, b0, 0.0f);
        push(o1, b1, 0.0f);
    }
}

void AppendStar(std::vector<BarVertex>& out, PanelSurface const& surface, float cx, float cy, float rx,
    float ry, D3DCOLOR color)
{
    constexpr int POINTS = 5;
    ScreenPoint outer[POINTS], inner[POINTS];
    for (int i = 0; i < POINTS; ++i) {
        float const ao = PI_F * 0.5f + i * 2.0f * PI_F / POINTS;
        float const ai = ao + PI_F / POINTS;
        outer[i] = surface.Map(cx + std::cos(ao) * rx, cy + std::sin(ao) * ry);
        inner[i] = surface.Map(cx + std::cos(ai) * rx * 0.4f, cy + std::sin(ai) * ry * 0.4f);
    }
    ScreenPoint const center = surface.Map(cx, cy);

    // Feather only the outline: each fan triangle carries the perpendicular distance to
    // its boundary edge (0 at the two boundary vertices), the radial edges stay solid.
    auto push = [&](ScreenPoint const& p, float to_edge) {
        BarVertex vertex;
        vertex.x = p.x;
        vertex.y = p.y;
        vertex.z = 0.0f;
        vertex.rhw = 1.0f;
        vertex.color = color;
        vertex.u = to_edge;
        vertex.v = 64.0f;
        vertex.u2 = 64.0f;
        vertex.v2 = 64.0f;
        out.push_back(vertex);
    };
    auto line_distance = [](ScreenPoint const& a, ScreenPoint const& b, ScreenPoint const& p) {
        float const ex = b.x - a.x, ey = b.y - a.y;
        float const length = std::sqrt(ex * ex + ey * ey);
        if (length < 1e-4f) return 0.0f;
        return std::fabs(-ey * (p.x - a.x) + ex * (p.y - a.y)) / length;
    };

    for (int i = 0; i < POINTS; ++i) {
        int const j = (i + 1) % POINTS;
        push(outer[i], 0.0f);
        push(inner[i], 0.0f);
        push(center, line_distance(outer[i], inner[i], center));

        push(inner[i], 0.0f);
        push(outer[j], 0.0f);
        push(center, line_distance(inner[i], outer[j], center));
    }
}

void AppendArcScreen(std::vector<BarVertex>& out, float cx, float cy, float radius, float thickness,
    float center_angle, float half_angle, D3DCOLOR color)
{
    float const half = std::max(0.0f, half_angle);
    if (half <= 0.0f || thickness <= 0.0f) return;

    float const inner = std::max(0.0f, radius - thickness * 0.5f);
    float const outer = radius + thickness * 0.5f;
    float const step_radians = 4.0f * PI_F / 180.0f;
    int const segments = std::max(2, static_cast<int>(std::ceil(half / step_radians)));
    float const a0 = center_angle - half;
    float const a1 = center_angle + half;
    float const step = (2.0f * half) / static_cast<float>(segments);

    // Encode the band's four boundaries (inner/outer radius, the two caps) rather than the
    // triangulation edges, so adjacent segments blend seamlessly and the arc reads anti-aliased.
    auto vertex_at = [&](float angle, float rho) {
        BarVertex vertex;
        vertex.x = cx + std::sin(angle) * rho;
        vertex.y = cy - std::cos(angle) * rho;
        vertex.z = 0.0f;
        vertex.rhw = 1.0f;
        vertex.color = color;
        vertex.u = rho - inner;
        vertex.v = outer - rho;
        vertex.u2 = std::fabs(rho * std::sin(angle - a0));
        vertex.v2 = std::fabs(rho * std::sin(a1 - angle));
        return vertex;
    };

    for (int i = 0; i < segments; ++i) {
        float const angle0 = a0 + step * i;
        float const angle1 = a0 + step * (i + 1);
        BarVertex const i0 = vertex_at(angle0, inner);
        BarVertex const o0 = vertex_at(angle0, outer);
        BarVertex const i1 = vertex_at(angle1, inner);
        BarVertex const o1 = vertex_at(angle1, outer);
        out.push_back(i0);
        out.push_back(o0);
        out.push_back(i1);
        out.push_back(i1);
        out.push_back(o0);
        out.push_back(o1);
    }
}

void AppendPointerScreen(std::vector<BarVertex>& out, float cx, float cy, float radius, float angle,
    float size, float width_ratio, D3DCOLOR color)
{
    if (size <= 0.0f) return;

    // Isosceles triangle: base sits on the arc (radius), tip points outward, so the arc band
    // covers the base and no hollow notch is left when the arc is shorter than the pointer.
    // `width_ratio` (base width / length) makes it adjustable and can be small for a slender look.
    float const w = size * std::max(0.0f, width_ratio) * 0.5f;
    ScreenPoint const base { cx + std::sin(angle) * radius, cy - std::cos(angle) * radius };
    ScreenPoint const outward { std::sin(angle), -std::cos(angle) };
    ScreenPoint const lateral { std::cos(angle), std::sin(angle) };
    auto place = [&](float lat, float along) {
        return ScreenPoint { base.x + lateral.x * lat + outward.x * along,
            base.y + lateral.y * lat + outward.y * along };
    };

    ScreenPoint const tip = place(0.0f, size);
    ScreenPoint const back_right = place(w, 0.0f);
    ScreenPoint const back_left = place(-w, 0.0f);

    AppendAATri(out, tip, back_right, back_left, color);
}

DeviceStateGuard::DeviceStateGuard(IDirect3DDevice9* device)
    : m_device(device)
{
    Save();
}

DeviceStateGuard::~DeviceStateGuard() { Restore(); }

void DeviceStateGuard::Save()
{
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &m_rw_texture_raster);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &m_rw_ztest);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &m_rw_zwrite);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &m_rw_vertex_alpha);
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &m_rw_fog);
    RwRenderStateGet(rwRENDERSTATESRCBLEND, &m_rw_src_blend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, &m_rw_dest_blend);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &m_rw_cull_mode);

    m_device->GetVertexShader(&m_vertex_shader);
    m_device->GetPixelShader(&m_pixel_shader);
    m_device->GetVertexDeclaration(&m_vertex_declaration);
    m_device->GetFVF(&m_fvf);
    m_device->GetTexture(0, &m_texture);
    m_device->GetStreamSource(0, &m_stream, &m_stream_offset, &m_stream_stride);
    m_device->GetRenderTarget(0, &m_render_target);
    m_device->GetViewport(&m_viewport);

    m_device->GetRenderState(D3DRS_ZENABLE, &m_render_states[0]);
    m_device->GetRenderState(D3DRS_ZWRITEENABLE, &m_render_states[1]);
    m_device->GetRenderState(D3DRS_CULLMODE, &m_render_states[2]);
    m_device->GetRenderState(D3DRS_LIGHTING, &m_render_states[3]);
    m_device->GetRenderState(D3DRS_FOGENABLE, &m_render_states[4]);
    m_device->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_render_states[5]);
    m_device->GetRenderState(D3DRS_SRCBLEND, &m_render_states[6]);
    m_device->GetRenderState(D3DRS_DESTBLEND, &m_render_states[7]);
    m_device->GetRenderState(D3DRS_ALPHATESTENABLE, &m_render_states[8]);
    m_device->GetRenderState(D3DRS_SCISSORTESTENABLE, &m_render_states[9]);

    m_device->GetTextureStageState(0, D3DTSS_COLOROP, &m_texture_stages[0]);
    m_device->GetTextureStageState(0, D3DTSS_COLORARG1, &m_texture_stages[1]);
    m_device->GetTextureStageState(0, D3DTSS_COLORARG2, &m_texture_stages[2]);
    m_device->GetTextureStageState(0, D3DTSS_ALPHAOP, &m_texture_stages[3]);
    m_device->GetTextureStageState(0, D3DTSS_ALPHAARG1, &m_texture_stages[4]);
    m_device->GetTextureStageState(0, D3DTSS_ALPHAARG2, &m_texture_stages[5]);

    m_device->GetSamplerState(0, D3DSAMP_MINFILTER, &m_sampler_states[0]);
    m_device->GetSamplerState(0, D3DSAMP_MAGFILTER, &m_sampler_states[1]);
    m_device->GetSamplerState(0, D3DSAMP_MIPFILTER, &m_sampler_states[2]);
    m_device->GetSamplerState(0, D3DSAMP_ADDRESSU, &m_sampler_states[3]);
    m_device->GetSamplerState(0, D3DSAMP_ADDRESSV, &m_sampler_states[4]);
}

void DeviceStateGuard::Restore()
{
    if (m_render_target) {
        m_device->SetRenderTarget(0, m_render_target);
        m_render_target->Release();
    }
    m_device->SetViewport(&m_viewport);

    m_device->SetVertexShader(m_vertex_shader);
    m_device->SetPixelShader(m_pixel_shader);
    if (m_vertex_declaration)
        m_device->SetVertexDeclaration(m_vertex_declaration);
    else
        m_device->SetFVF(m_fvf);
    m_device->SetTexture(0, m_texture);
    m_device->SetStreamSource(0, m_stream, m_stream_offset, m_stream_stride);

    m_device->SetRenderState(D3DRS_ZENABLE, m_render_states[0]);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, m_render_states[1]);
    m_device->SetRenderState(D3DRS_CULLMODE, m_render_states[2]);
    m_device->SetRenderState(D3DRS_LIGHTING, m_render_states[3]);
    m_device->SetRenderState(D3DRS_FOGENABLE, m_render_states[4]);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, m_render_states[5]);
    m_device->SetRenderState(D3DRS_SRCBLEND, m_render_states[6]);
    m_device->SetRenderState(D3DRS_DESTBLEND, m_render_states[7]);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, m_render_states[8]);
    m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, m_render_states[9]);

    m_device->SetTextureStageState(0, D3DTSS_COLOROP, m_texture_stages[0]);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, m_texture_stages[1]);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, m_texture_stages[2]);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, m_texture_stages[3]);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, m_texture_stages[4]);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, m_texture_stages[5]);

    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, m_sampler_states[0]);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, m_sampler_states[1]);
    m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, m_sampler_states[2]);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, m_sampler_states[3]);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, m_sampler_states[4]);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, m_rw_texture_raster);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, m_rw_ztest);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, m_rw_zwrite);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, m_rw_vertex_alpha);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, m_rw_fog);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, m_rw_src_blend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, m_rw_dest_blend);
    RwRenderStateSet(rwRENDERSTATECULLMODE, m_rw_cull_mode);

    if (m_vertex_shader)
        m_vertex_shader->Release();
    if (m_pixel_shader)
        m_pixel_shader->Release();
    if (m_vertex_declaration)
        m_vertex_declaration->Release();
    if (m_texture)
        m_texture->Release();
    if (m_stream)
        m_stream->Release();
}
}
