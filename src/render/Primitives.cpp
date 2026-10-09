#include "render/Primitives.hpp"

#include "core/Math.hpp"

#include <algorithm>
#include <cmath>

namespace division::render {
namespace {
    constexpr float AASOFTNESS = 1.0f;
    constexpr float SOLID_COVERAGE = 64.0f;

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

void DrawBackdrop(IDirect3DDevice9* device, ScreenPoint const (&points)[4], float const (&uv)[4][2],
    float opacity)
{
    // Colour stage passes the blur texture through untouched (SELECTARG1), so no tint
    // colour is multiplied in; alpha comes from the vertex diffuse (SELECTARG2).
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    unsigned int const a = static_cast<unsigned int>(std::clamp(opacity, 0.0f, 1.0f) * 255.0f + 0.5f) & 0xffu;
    Vertex vertices[4];
    for (int i = 0; i < 4; ++i) {
        vertices[i].x = points[i].x;
        vertices[i].y = points[i].y;
        vertices[i].z = 0.0f;
        vertices[i].rhw = 1.0f;
        vertices[i].color = D3DCOLOR_ARGB(a, 255, 255, 255); // only A is read
        vertices[i].u = uv[i][0];
        vertices[i].v = uv[i][1];
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));

    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
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
        vertex.u2 = SOLID_COVERAGE;
        vertex.v2 = SOLID_COVERAGE;
        out.push_back(vertex);
    };
    auto edge = [&](float angle, float ratio) {
        return surface.Map(cx + std::cos(angle) * rx * ratio, cy + std::sin(angle) * ry * ratio);
    };

    for (int i = 0; i < SEGMENTS; ++i) {
        float const a0 = i * 2.0f * core::PI_F / SEGMENTS;
        float const a1 = (i + 1) * 2.0f * core::PI_F / SEGMENTS;
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
        float const ao = core::PI_F * 0.5f + i * 2.0f * core::PI_F / POINTS;
        float const ai = ao + core::PI_F / POINTS;
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
        vertex.v = SOLID_COVERAGE;
        vertex.u2 = SOLID_COVERAGE;
        vertex.v2 = SOLID_COVERAGE;
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
    float const step_radians = 4.0f * core::PI_F / 180.0f;
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

}