#include "hud/widgets/Info.hpp"

#include "plugin.h"

#include "CPlayerData.h"
#include "CWorld.h"
#include "CWanted.h"
#include "CClock.h"

#include <algorithm>
#include <cstdio>
#include <string_view>

#include "core/Config.hpp"
#include "hud/widgets/Layout.hpp"

namespace division::hud {
using namespace render;

namespace {
    bool DrawString(IDirect3DDevice9* device, TextRenderer& text, PanelSurface const& surface, std::string_view str,
        float x_left, float y_center, float glyph_height, D3DCOLOR color)
    {
        int tex_w = 0;
        int tex_h = 0;
        IDirect3DTexture9* texture = text.Get(device, str, tex_w, tex_h);
        if (!texture || tex_w <= 0 || tex_h <= 0) return false;

        float const glyph_width = glyph_height * (static_cast<float>(tex_w) / static_cast<float>(tex_h)) * UnitAspect();
        ScreenPoint const tl = surface.Map(x_left, y_center + glyph_height * 0.5f);
        ScreenPoint const tr = surface.Map(x_left + glyph_width, y_center + glyph_height * 0.5f);
        ScreenPoint const bl = surface.Map(x_left, y_center - glyph_height * 0.5f);
        ScreenPoint const br = surface.Map(x_left + glyph_width, y_center - glyph_height * 0.5f);

        device->SetTexture(0, texture);
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };

        if (core::cfg.PanelHudTextShadowOffset > 0.0f) {
            float const o = core::cfg.PanelHudTextShadowOffset;
            float const alpha = ((color >> 24) & 0xff) / 255.0f;
            ScreenPoint const shadow[4] = {
                { tl.x + o, tl.y + o }, { tr.x + o, tr.y + o },
                { bl.x + o, bl.y + o }, { br.x + o, br.y + o },
            };
            DrawTexturedQuad(device, shadow, uv, ScaleColor(core::cfg.PanelHudTextShadowColor, alpha));
        }

        DrawTexturedQuad(device, quad, uv, color);
        return true;
    }
}

void DrawInfoIcons(IDirect3DDevice9* device, PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha, std::vector<BarVertex>& vertices)
{
    float const unit_aspect = UnitAspect();
    D3DCOLOR const color = ScaleColor(core::cfg.PanelHudIconColor, panel_alpha);

    auto icon_radius = [&](Rect const& r) {
        return (r.y1 - r.y0) * core::cfg.PanelHudIconSize * 0.5f;
    };

    vertices.clear();
    vertices.reserve(768);

    {
        Rect const& r = panels.resource;
        float const cx = r.x0 + (r.x1 - r.x0) * core::cfg.PanelClockIconX;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const ry = icon_radius(r);
        float const rx = ry * unit_aspect;
        AppendRing(vertices, surface, cx, cy, rx, ry, 0.87f, color);
        AppendAARect(vertices, surface, cx - rx * 0.08f, cy, cx + rx * 0.08f, cy + ry * 0.52f, color);
        AppendAARect(vertices, surface, cx, cy - ry * 0.055f, cx + rx * 0.70f, cy + ry * 0.055f, color);
    }

    {
        Rect const& r = panels.resource;
        float const cx = r.x0 + (r.x1 - r.x0) * core::cfg.PanelWantedIconX;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const ry = icon_radius(r);
        AppendStar(vertices, surface, cx, cy, ry * unit_aspect, ry, color);
    }

    if (!vertices.empty())
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(),
            sizeof(BarVertex));
}

void DrawInfoText(IDirect3DDevice9* device, TextRenderer& text, PanelSurface const& surface,
    PanelRects const& panels, float panel_alpha)
{
    D3DCOLOR const color = ScaleColor(core::cfg.PanelHudTextColor, panel_alpha);
    CPlayerInfo& info = CWorld::Players[CWorld::PlayerInFocus];

    {
        Rect const& r = panels.skill;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const text_height = (r.y1 - r.y0) * core::cfg.PanelHudTextSize;
        float const icon_height = (r.y1 - r.y0) * core::cfg.PanelHudIconSize;
        char buffer[32] { };
        std::snprintf(buffer, sizeof(buffer), "%0*d", std::max(1, core::cfg.PanelMoneyDigits), info.m_nMoney);
        DrawString(device, text, surface, "$", r.x0 + (r.x1 - r.x0) * core::cfg.PanelSkillIconX, cy, icon_height,
            color);
        DrawString(device, text, surface, buffer, r.x0 + (r.x1 - r.x0) * core::cfg.PanelSkillValueX, cy, text_height,
            color);
    }

    {
        Rect const& r = panels.resource;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const text_height = (r.y1 - r.y0) * core::cfg.PanelHudTextSize;
        char buffer[16] { };
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", CClock::ms_nGameClockHours, CClock::ms_nGameClockMinutes);
        DrawString(device, text, surface, buffer, r.x0 + (r.x1 - r.x0) * core::cfg.PanelClockValueX, cy, text_height,
            color);
    }

    {
        Rect const& r = panels.resource;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const text_height = (r.y1 - r.y0) * core::cfg.PanelHudTextSize;
        unsigned int wanted = 0;
        if (info.m_PlayerData.m_pWanted)
            wanted = info.m_PlayerData.m_pWanted->m_nWantedLevel;
        char buffer[16] { };
        std::snprintf(buffer, sizeof(buffer), "%u", wanted);
        DrawString(device, text, surface, buffer, r.x0 + (r.x1 - r.x0) * core::cfg.PanelWantedValueX, cy, text_height,
            color);
    }
}

}