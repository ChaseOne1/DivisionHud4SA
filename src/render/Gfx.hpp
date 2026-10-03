#pragma once

#include <d3d9.h>
#include <string>
#include <vector>

namespace division::render {
struct ScreenPoint
{
    float x = 0.0f;
    float y = 0.0f;
};

// Maps normalized local panel coordinates (x,y in [0,1], y up) onto a screen-space quad.
class PanelSurface
{
public:
    void SetCorners(ScreenPoint const& top_left, ScreenPoint const& top_right,
        ScreenPoint const& bottom_right, ScreenPoint const& bottom_left);
    ScreenPoint Map(float nx, float ny) const;

private:
    ScreenPoint m_top_left { }, m_top_right { }, m_bottom_right { }, m_bottom_left { };
};

struct Vertex
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
};

struct BarVertex
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
    float u2, v2;
};

constexpr DWORD VERTEX_FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
constexpr DWORD BAR_FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2;

D3DCOLOR ScaleColor(D3DCOLOR color, float extra);

// Reads a text resource from the plugin's DivisionHud4SA resource folder (relative path),
// returning `fallback` if it cannot be read.
std::string LoadResourceText(char const* relative_path, char const* fallback);

void DrawTexturedQuad(IDirect3DDevice9* device, ScreenPoint const (&points)[4], float const (&uv)[4][2],
    D3DCOLOR color);

IDirect3DPixelShader9* CreateLineShader(IDirect3DDevice9* device);
IDirect3DPixelShader9* CreateBarShader(IDirect3DDevice9* device);
IDirect3DPixelShader9* CreateWaveShader(IDirect3DDevice9* device);
IDirect3DPixelShader9* CompilePixelShader(IDirect3DDevice9* device, char const* source);

void DrawLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to, float thickness,
    D3DCOLOR color);
void DrawEdgeLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to,
    ScreenPoint const& interior, float thickness, D3DCOLOR color);

void AppendSolidTri(std::vector<BarVertex>& out, ScreenPoint const& a, ScreenPoint const& b,
    ScreenPoint const& c, D3DCOLOR color);
// Anti-aliased triangle (soft edges via the bar shader's edge-distance coverage).
void AppendAATri(std::vector<BarVertex>& out, ScreenPoint const& a, ScreenPoint const& b,
    ScreenPoint const& c, D3DCOLOR color);
void AppendAARect(std::vector<BarVertex>& out, PanelSurface const& surface, float x0, float y0, float x1,
    float y1, D3DCOLOR color);
void AppendSolidRect(std::vector<BarVertex>& out, PanelSurface const& surface, float x0, float y0, float x1,
    float y1, D3DCOLOR color);
void AppendRing(std::vector<BarVertex>& out, PanelSurface const& surface, float cx, float cy, float rx,
    float ry, float inner_ratio, D3DCOLOR color);
void AppendStar(std::vector<BarVertex>& out, PanelSurface const& surface, float cx, float cy, float rx,
    float ry, D3DCOLOR color);

// Screen-space helpers for the damage indicators. Angles are measured clockwise from screen-up,
// i.e. direction(angle) = (sin angle, -cos angle).
void AppendArcScreen(std::vector<BarVertex>& out, float cx, float cy, float radius, float thickness,
    float center_angle, float half_angle, D3DCOLOR color);
void AppendPointerScreen(std::vector<BarVertex>& out, float cx, float cy, float radius, float angle,
    float size, float width_ratio, D3DCOLOR color);

// Saves and restores the D3D9 + RenderWare device state that the HUD overrides.
class DeviceStateGuard
{
public:
    explicit DeviceStateGuard(IDirect3DDevice9* device);
    ~DeviceStateGuard();

    DeviceStateGuard(DeviceStateGuard const&) = delete;
    DeviceStateGuard& operator=(DeviceStateGuard const&) = delete;

private:
    void Save();
    void Restore();

    IDirect3DDevice9* m_device;

    void* m_rw_texture_raster = nullptr;
    void* m_rw_ztest = nullptr;
    void* m_rw_zwrite = nullptr;
    void* m_rw_vertex_alpha = nullptr;
    void* m_rw_fog = nullptr;
    void* m_rw_src_blend = nullptr;
    void* m_rw_dest_blend = nullptr;
    void* m_rw_cull_mode = nullptr;

    IDirect3DVertexShader9* m_vertex_shader = nullptr;
    IDirect3DPixelShader9* m_pixel_shader = nullptr;
    IDirect3DVertexDeclaration9* m_vertex_declaration = nullptr;
    DWORD m_fvf = 0;
    IDirect3DBaseTexture9* m_texture = nullptr;
    IDirect3DVertexBuffer9* m_stream = nullptr;
    UINT m_stream_offset = 0;
    UINT m_stream_stride = 0;
    IDirect3DSurface9* m_render_target = nullptr;
    D3DVIEWPORT9 m_viewport { };

    DWORD m_render_states[10] { };
    DWORD m_texture_stages[6] { };
    DWORD m_sampler_states[5] { };
};
}
