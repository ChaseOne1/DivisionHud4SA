#include "render/Pipeline.hpp"

#include "render/Surface.hpp"

#include "plugin.h"

namespace division::render {

void ApplyScreenPipeline(IDirect3DDevice9* device)
{
    device->SetVertexShader(nullptr);
    device->SetPixelShader(nullptr);
    device->SetVertexDeclaration(nullptr);
    device->SetFVF(VERTEX_FVF);
    device->SetStreamSource(0, nullptr, 0, 0);
    device->SetTexture(0, nullptr);

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    // Post-effects (e.g. the fire heat-haze stencil mask) can leave the stencil test and
    // colour-write mask in a state that silently discards our screen-space geometry.
    // We never use either, so force them back to a known-good state.
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    device->SetRenderState(D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
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
    m_device->GetRenderState(D3DRS_STENCILENABLE, &m_render_states[10]);
    m_device->GetRenderState(D3DRS_COLORWRITEENABLE, &m_render_states[11]);
    m_device->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, &m_render_states[12]);

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
    m_device->SetRenderState(D3DRS_STENCILENABLE, m_render_states[10]);
    m_device->SetRenderState(D3DRS_COLORWRITEENABLE, m_render_states[11]);
    m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, m_render_states[12]);

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