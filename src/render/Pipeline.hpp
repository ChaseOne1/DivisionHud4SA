#pragma once

#include <d3d9.h>

namespace division::render {

// Applies the fixed-function 2D pipeline the on-screen HUD relies on: no vertex /
// pixel shaders, the VERTEX_FVF layout, alpha blending, and the texture-stage /
// sampler defaults. The caller holds a DeviceStateGuard so the game state is
// restored on scope exit.
void ApplyScreenPipeline(IDirect3DDevice9* device);

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

    DWORD m_render_states[13] { };
    DWORD m_texture_stages[6] { };
    DWORD m_sampler_states[5] { };
};
}