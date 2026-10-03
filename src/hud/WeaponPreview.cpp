#include "hud/WeaponPreview.hpp"

#include "plugin.h"

#include "common.h"
#include "CPed.h"
#include "CWeapon.h"
#include "CWeaponInfo.h"
#include "CBaseModelInfo.h"
#include "CModelInfo.h"
#include "CTxdStore.h"
#include "TxdDef.h"
#include "CKeyGen.h"
#include "CHud.h"
#include "CTimer.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

#include "core/Config.hpp"
#include "render/Gfx.hpp"

namespace division::hud {
using namespace render;

namespace {
    constexpr float TIME_STEP_TO_SECONDS = 0.02f;
    constexpr float MIN_FADE_DURATION = 0.02f;
    constexpr float MAX_FADE_STEP = 0.5f;

    float AdvanceFade(float alpha, float target, float dt, float fade_in, float fade_out)
    {
        float const fade = (alpha < target) ? std::max(MIN_FADE_DURATION, fade_in) : std::max(MIN_FADE_DURATION, fade_out);
        float const step = std::min(MAX_FADE_STEP, dt / fade);
        if (alpha < target) return std::min(1.0f, alpha + step);
        if (alpha > target) return std::max(0.0f, alpha - step);
        return alpha;
    }

    bool IsUsableSlot(CPed* ped, int slot)
    {
        if (slot == 0) return true; // fists are always switchable
        CWeapon const& weapon = ped->m_aWeapons[slot];
        if (weapon.m_eWeaponType == WEAPONTYPE_UNARMED) return false;
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType, 1);
        if (!info || static_cast<int>(info->m_nSlot) != slot) return false;
        // A clip-fed weapon with no ammo left is no longer owned; this hides stale
        // slots left behind after death / weapon clearing.
        if (info->m_nAmmoClip > 0 && weapon.m_nAmmoInClip == 0 && weapon.m_nAmmoTotal == 0) return false;
        return true;
    }

    // Draw icons with direct D3D, never RW state/CSprite: drawingEvent runs before
    // CHud::Draw, so leaked RenderWare state corrupts the game's own HUD.
    // Native texture = raster + [RasterExtOffset global]; D3D9 keeps it there.
    IDirect3DTexture9* RasterToTexture(RwRaster* raster)
    {
        if (!raster) return nullptr;
        uintptr_t const type = raster->cType & rwRASTERTYPEMASK;
        if (type != rwRASTERTYPETEXTURE && type != rwRASTERTYPECAMERATEXTURE && type != 0)
            return nullptr;
        uintptr_t const offset = *reinterpret_cast<uintptr_t const*>(0xB4E9E0);
        if (offset == 0) return nullptr;
        return *reinterpret_cast<IDirect3DTexture9**>(reinterpret_cast<char*>(raster) + offset);
    }

    // The game stores each weapon's HUD icon as a `<modelKey>ICON` texture inside
    // the weapon model's TXD (see CHud::DrawWeaponIcon). Unarmed falls back to the
    // fist sprite.
    IDirect3DTexture9* ResolveWeaponIcon(eWeaponType type)
    {
        RwTexture* texture = nullptr;
        // Skill 1 is the base weapon row; skill 0 indexes a different weapon entry
        // (GetWeaponInfo adds 25), which showed the wrong icon.
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(type, 1);
        if (info && info->m_nModelId > 0) {
            CBaseModelInfo* model = CModelInfo::GetModelInfo(info->m_nModelId);
            if (model && model->m_nTxdIndex >= 0 && CTxdStore::ms_pTxdPool) {
                TxdDef* def = CTxdStore::ms_pTxdPool->GetAt(model->m_nTxdIndex);
                if (def && def->m_pRwDictionary) {
                    unsigned int const key = CKeyGen::AppendStringToKey(model->m_nKey, "ICON");
                    texture = RwTexDictionaryFindHashNamedTexture(def->m_pRwDictionary, key);
                }
            }
        }
        if (!texture && type == WEAPONTYPE_UNARMED && CHud::Sprites)
            texture = CHud::Sprites[HUDSPRITE_FIST].m_pTexture;
        return RasterToTexture(texture ? texture->raster : nullptr);
    }

    void DrawIconTexture(IDirect3DDevice9* device, IDirect3DTexture9* texture, float cx, float cy, float max_w,
        float max_h, float alpha)
    {
        if (!texture || max_w <= 0.0f || max_h <= 0.0f) return;

        D3DSURFACE_DESC desc { };
        if (FAILED(texture->GetLevelDesc(0, &desc)) || desc.Width == 0 || desc.Height == 0) return;

        float const aspect = static_cast<float>(desc.Width) / static_cast<float>(desc.Height);
        float w = max_w;
        float h = w / aspect;
        if (h > max_h) {
            h = max_h;
            w = h * aspect;
        }
        if (w <= 0.0f || h <= 0.0f) return;

        // Icon textures carry their transparency in the alpha channel.
        device->SetTexture(0, texture);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        ScreenPoint const tl { cx - w * 0.5f, cy - h * 0.5f };
        ScreenPoint const tr { cx + w * 0.5f, cy - h * 0.5f };
        ScreenPoint const bl { cx - w * 0.5f, cy + h * 0.5f };
        ScreenPoint const br { cx + w * 0.5f, cy + h * 0.5f };
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };
        DrawTexturedQuad(device, quad, uv, D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, alpha));
    }

    void DrawCenteredString(IDirect3DDevice9* device, TextRenderer& text, std::string const& str, float cx,
        float cy, float max_w, float max_h, float size_fraction, D3DCOLOR color)
    {
        int tex_w = 0;
        int tex_h = 0;
        IDirect3DTexture9* texture = text.Get(device, str, tex_w, tex_h);
        if (!texture || tex_w <= 0 || tex_h <= 0 || max_w <= 0.0f || max_h <= 0.0f) return;

        float const aspect = static_cast<float>(tex_w) / static_cast<float>(tex_h);
        float h = max_h * std::clamp(size_fraction, 0.05f, 1.0f);
        float w = h * aspect;
        if (w > max_w) {
            w = max_w;
            h = w / aspect;
        }
        if (w <= 0.0f || h <= 0.0f) return;

        ScreenPoint const tl { cx - w * 0.5f, cy - h * 0.5f };
        ScreenPoint const tr { cx + w * 0.5f, cy - h * 0.5f };
        ScreenPoint const bl { cx - w * 0.5f, cy + h * 0.5f };
        ScreenPoint const br { cx + w * 0.5f, cy + h * 0.5f };
        device->SetTexture(0, texture);
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };
        DrawTexturedQuad(device, quad, uv, color);
    }
}

void WeaponPreview::Update()
{
    CPed* ped = FindPlayerPed(0);
    bool const in_vehicle = FindPlayerVehicle(0, false) != nullptr;

    float dt = CTimer::ms_fTimeStep * TIME_STEP_TO_SECONDS;
    if (!(dt > 0.0f))
        dt = 1.0f / 60.0f;

    bool const eligible = ped && !in_vehicle;
    if (eligible) {
        if (!m_has_prev_slot) {
            m_prev_slot = ped->m_nSelectedWepSlot;
            m_has_prev_slot = true;
        } else if (static_cast<int>(ped->m_nSelectedWepSlot) != m_prev_slot) {
            m_prev_slot = ped->m_nSelectedWepSlot;
            m_hold = core::WeaponPreviewHold;
        }
    } else {
        m_has_prev_slot = false;
    }

    m_hold = std::max(0.0f, m_hold - dt);
    float const target = (eligible && m_hold > 0.0f) ? 1.0f : 0.0f;
    m_alpha = AdvanceFade(m_alpha, target, dt, core::WeaponPreviewFadeIn, core::WeaponPreviewFadeOut);

    // Once fully hidden, forget the animation state so the next appearance snaps in.
    if (target <= 0.0f && m_alpha < 0.01f) {
        m_scroll_init = false;
        m_bias = 1.0f;
        m_bias_from = 1.0f;
        m_tween = 1.0f;
        m_prev_current = -1;
    }
}

void WeaponPreview::Draw(IDirect3DTexture9* blurred)
{
    if (m_alpha < 0.01f || !blurred) return;

    CPed* ped = FindPlayerPed(0);
    if (!ped || FindPlayerVehicle(0, false)) return;

    IDirect3DDevice9* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    if (!device) return;

    int const screen_width = RsGlobal.maximumWidth;
    int const screen_height = RsGlobal.maximumHeight;
    if (screen_width <= 0 || screen_height <= 0) return;
    float const screen_w = static_cast<float>(screen_width);
    float const screen_h = static_cast<float>(screen_height);

    int slots[13];
    int n = 0;
    for (int s = 0; s < 13; ++s) {
        if (IsUsableSlot(ped, s)) slots[n++] = s;
    }
    if (n < 2) return;

    int current = -1;
    for (int i = 0; i < n; ++i) {
        if (slots[i] == static_cast<int>(ped->m_nSelectedWepSlot)) {
            current = i;
            break;
        }
    }
    if (current < 0) return;

    int const max_count = std::clamp(core::WeaponPreviewMaxCount, 1, 5);
    int const k = std::min(max_count, n);

    float icon_h = std::max(0.005f, core::WeaponPreviewIconHeight) * screen_h;
    float icon_region_h = icon_h * (1.0f + 2.0f * std::max(0.0f, core::WeaponPreviewIconPad));
    float ammo_region_h = icon_h * std::max(0.0f, core::WeaponPreviewAmmoRatio);
    float panel_h = icon_region_h + ammo_region_h;
    float panel_w = panel_h * std::max(0.2f, core::WeaponPreviewPanelAspect);
    float gap = std::max(0.0f, core::WeaponPreviewGap) * screen_w;

    float row_w = k * panel_w + (k - 1) * gap;
    float const max_w = std::max(0.1f, core::WeaponPreviewMaxWidth) * screen_w;
    if (row_w > max_w && row_w > 0.0f) {
        float const scale = max_w / row_w;
        icon_region_h *= scale;
        ammo_region_h *= scale;
        panel_h *= scale;
        panel_w *= scale;
        gap *= scale;
    }

    float const center_x = core::WeaponPreviewCenterX * screen_w;
    float const center_y = core::WeaponPreviewCenterY * screen_h;
    float const active_scale = std::max(1.0f, core::WeaponPreviewActiveScale);

    float const step = panel_w + gap;
    float const panel_top = center_y - panel_h * 0.5f;
    float const panel_bottom = center_y + panel_h * 0.5f;

    // Frame step for the animation, clamped so a frame hitch cannot teleport the reel.
    float dt = CTimer::ms_fTimeStep * TIME_STEP_TO_SECONDS;
    if (!(dt > 0.0f))
        dt = 1.0f / 60.0f;
    if (dt > 0.05f)
        dt = 0.05f;

    // One-directional reel: weapons sit on an infinite strip (the list repeated) and
    // `m_scroll` moves to the selected slot over a fixed-duration smoothstep, so the
    // panel leaving on the left fades out over the same time a new one fades in on
    // the right (an exponential ease used to exit fast and enter slow).
    if (!m_scroll_init) {
        m_scroll = m_target = static_cast<float>(current);
        m_tween_from = m_scroll;
        m_tween = 1.0f;
        m_bias = m_bias_from = 1.0f;
        m_prev_current = current;
        m_scroll_init = true;
    } else if (current != m_prev_current) {
        int diff = ((current - m_prev_current) % n + n) % n;
        int delta = (diff <= n / 2) ? diff : diff - n;
        if (n == 2)
            delta = 1; // with two weapons both cycles are the same forward step
        m_tween_from = m_scroll;
        m_bias_from = m_bias;
        m_target += static_cast<float>(delta);
        m_tween = 0.0f;
        m_tween_dur = std::max(0.08f, std::fabs(static_cast<float>(delta)) / std::max(1.0f, core::WeaponPreviewSlideSpeed));
        m_prev_current = current;
    }
    if (m_tween < 1.0f)
        m_tween = std::min(1.0f, m_tween + dt / m_tween_dur);
    float const ease = m_tween * m_tween * (3.0f - 2.0f * m_tween);
    m_scroll = m_tween_from + (static_cast<float>(m_target) - m_tween_from) * ease;
    m_bias = m_bias_from + (-1.0f - m_bias_from) * ease; // only relevant with 2 weapons

    // Visible window in steps from the centre. With two weapons the single neighbour
    // sits on the right at first and eases to the left after a switch.
    float win_lo = 0.0f;
    float win_hi = 0.0f;
    if (n == 2) {
        float const center = 0.5f * m_bias;
        win_lo = center - 1.0f;
        win_hi = center + 1.0f;
    } else if (k % 2 == 0) {
        // Even panel count: offset the window so the antipodal weapon stays visible and
        // its strip jump happens off the edges.
        float const half = static_cast<float>(k) * 0.5f;
        win_lo = -half - 0.5f;
        win_hi = half - 0.5f;
    } else {
        float const radius = static_cast<float>(k) * 0.5f;
        win_lo = -radius;
        win_hi = radius;
    }
    float const edge = (n == 2) ? 0.35f : 0.5f;

    struct Panel
    {
        int slot = 0;
        float cx = 0.0f;
        float alpha = 1.0f;
        float act = 0.0f; // 0..1 closeness to the centre: drives highlight and icon size
    };

    // Smoothstep so both the edge fade and the centre emphasis ease in and out.
    auto smoothstep01 = [](float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    constexpr float ACT_RANGE = 0.9f;

    Panel panels[13];
    int panel_count = 0;
    for (int i = 0; i < n && panel_count < 13; ++i) {
        int const base = static_cast<int>(std::floor((m_scroll - static_cast<float>(i)) / static_cast<float>(n))) - 1;
        for (int mm = base; mm <= base + 2 && panel_count < 13; ++mm) {
            float const d = static_cast<float>(i + mm * n) - m_scroll;
            if (d < win_lo || d > win_hi)
                continue;
            float const dist = std::min(d - win_lo, win_hi - d);
            float alpha = 1.0f;
            if (dist < edge)
                alpha = smoothstep01(dist / edge);
            float const cx = center_x + d * step;
            if (cx < -panel_w || cx > screen_w + panel_w)
                continue;
            panels[panel_count].slot = slots[i];
            panels[panel_count].cx = cx;
            panels[panel_count].alpha = alpha;
            panels[panel_count].act = smoothstep01(1.0f - std::fabs(d) / ACT_RANGE);
            ++panel_count;
        }
    }

    DeviceStateGuard guard(device);

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

    float const tint_r = ((core::BackTint >> 16) & 0xff) / 255.0f;
    float const tint_g = ((core::BackTint >> 8) & 0xff) / 255.0f;
    float const tint_b = (core::BackTint & 0xff) / 255.0f;
    float const tint_a = (((core::BackTint >> 24) & 0xff) / 255.0f) * m_alpha;
    D3DCOLOR const tint = D3DCOLOR_COLORVALUE(tint_r, tint_g, tint_b, tint_a);

    // Triangle strip needs v1/v2 on a diagonal: tl,tr,bl,br. tl,tr,br,bl leaves a
    // triangular hole on one side and an overlap on the other.
    auto draw_quad = [&](float x0, float y0, float x1, float y1, D3DCOLOR color) {
        ScreenPoint const points[4] = { { x0, y0 }, { x1, y0 }, { x0, y1 }, { x1, y1 } };
        float const uv[4][2] = {
            { x0 / screen_w, y0 / screen_h },
            { x1 / screen_w, y0 / screen_h },
            { x0 / screen_w, y1 / screen_h },
            { x1 / screen_w, y1 / screen_h },
        };
        DrawTexturedQuad(device, points, uv, color);
    };

    // Backdrop (blurred) and, blended by proximity to the centre, the highlight fill.
    for (int i = 0; i < panel_count; ++i) {
        Panel const& p = panels[i];
        float const x0 = p.cx - panel_w * 0.5f;
        float const x1 = p.cx + panel_w * 0.5f;
        device->SetTexture(0, blurred);
        draw_quad(x0, panel_top, x1, panel_bottom, ScaleColor(tint, p.alpha));
        if (p.act > 0.001f) {
            device->SetTexture(0, nullptr);
            ScreenPoint const points[4] = {
                { x0, panel_top }, { x1, panel_top }, { x0, panel_bottom }, { x1, panel_bottom }
            };
            float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };
            DrawTexturedQuad(device, points, uv,
                ScaleColor(core::WeaponPreviewActiveColor, m_alpha * p.alpha * p.act));
        }
    }

    if (!m_line_shader)
        m_line_shader = CreateLineShader(device);
    device->SetPixelShader(m_line_shader);
    device->SetTexture(0, nullptr);
    float const thickness = std::max(0.5f, core::WeaponPreviewLineWidth);
    float const y_b = panel_bottom - ammo_region_h; // B: below the middle (icon above, ammo below)
    for (int i = 0; i < panel_count; ++i) {
        Panel const& p = panels[i];
        float const x0 = p.cx - panel_w * 0.5f;
        float const x1 = p.cx + panel_w * 0.5f;
        D3DCOLOR const line_color = ScaleColor(core::WeaponPreviewLineColor, m_alpha * p.alpha);
        DrawLine(device, { x0, panel_top }, { x1, panel_top }, thickness, line_color);       // A: top
        DrawLine(device, { x0, y_b }, { x1, y_b }, thickness, line_color);                   // B
        DrawLine(device, { x0, panel_bottom }, { x1, panel_bottom }, thickness, line_color); // C: bottom
    }

    device->SetPixelShader(nullptr);
    device->SetFVF(VERTEX_FVF);
    float const icon_top = panel_top;
    float const icon_bottom = panel_bottom - ammo_region_h;
    float const icon_cy = (icon_top + icon_bottom) * 0.5f;
    for (int i = 0; i < panel_count; ++i) {
        Panel const& p = panels[i];
        // Only the icon grows, and it eases with how centred the panel is.
        float const scale = 1.0f + (active_scale - 1.0f) * p.act;
        IDirect3DTexture9* texture = ResolveWeaponIcon(ped->m_aWeapons[p.slot].m_eWeaponType);
        DrawIconTexture(device, texture, p.cx, icon_cy, panel_w * 0.84f * scale,
            (icon_bottom - icon_top) * 0.82f * scale, m_alpha * p.alpha);
    }

    if (!m_text_initialised) {
        m_text.Initialize(PLUGIN_PATH("DivisionHud4SA/fonts/Borda Medium.ttf"), 128);
        m_text_initialised = true;
    }

    device->SetPixelShader(nullptr);
    device->SetFVF(VERTEX_FVF);
    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    float const text_cy = panel_bottom - ammo_region_h * 0.5f;
    for (int i = 0; i < panel_count; ++i) {
        Panel const& p = panels[i];
        CWeapon const& weapon = ped->m_aWeapons[p.slot];
        unsigned int const reserve = (weapon.m_nAmmoTotal > weapon.m_nAmmoInClip)
            ? (weapon.m_nAmmoTotal - weapon.m_nAmmoInClip)
            : 0u;
        char buffer[24] { };
        std::snprintf(buffer, sizeof(buffer), "%u/%u", weapon.m_nAmmoInClip, reserve);
        DrawCenteredString(device, m_text, buffer, p.cx, text_cy, panel_w * 0.9f, ammo_region_h,
            core::WeaponPreviewTextSize, ScaleColor(core::WeaponPreviewTextColor, m_alpha * p.alpha));
    }
}

void WeaponPreview::OnDeviceLost()
{
    m_text.OnDeviceLost();
    if (m_line_shader) {
        m_line_shader->Release();
        m_line_shader = nullptr;
    }
}

}
