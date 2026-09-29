#include "plugin.h"

#include "common.h"
#include "CPed.h"
#include "CPedIntelligence.h"
#include "CWeaponInfo.h"
#include "CPlayerData.h"
#include "CStats.h"
#include "CWorld.h"
#include "CWanted.h"
#include "CClock.h"
#include "CCamera.h"
#include "CTimer.h"
#include "CSprite.h"
#include "ePedBones.h"
#include "eCamMode.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>
#include <d3d9.h>
#include <d3dcompiler.h>

#include "Panel3D.h"
#include "Config.h"
#include "Layout.h"

namespace division {
namespace {
    constexpr float kTimeStepToSeconds = 0.02f;
    constexpr float kStaminaFloor = -150.0f;
    float const kPi = static_cast<float>(PI);
    constexpr DWORD kFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    constexpr DWORD kBarFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2;

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

    struct ScreenPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    CVector CameraRight() { return TheCamera.m_mCameraMatrix.GetRight(); }
    CVector CameraForward() { return TheCamera.m_mCameraMatrix.GetForward(); }

    float WrapAngle(float angle)
    {
        while (angle > kPi) angle -= 2.0f * kPi;
        while (angle < -kPi) angle += 2.0f * kPi;
        return angle;
    }

    bool Project(CVector const& world, ScreenPoint& out)
    {
        RwV3d projected { };
        float w = 0.0f, h = 0.0f;
        if (!CSprite::CalcScreenCoors(world, &projected, &w, &h, false, false))
            return false;
        out.x = projected.x;
        out.y = projected.y;
        return true;
    }

    bool IsOccupiedSlot(CPed* ped, int slot)
    {
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(ped->m_aWeapons[slot].m_eWeaponType, 0);
        return info && static_cast<int>(info->m_nSlot) == slot;
    }

    int FindWeaponSlot(CPed* ped, int from, int step)
    {
        int const count = static_cast<int>(std::size(ped->m_aWeapons));
        for (int i = 1; i < count; ++i) {
            int const slot = ((from + step * i) % count + count) % count;
            if (IsOccupiedSlot(ped, slot))
                return slot;
        }
        return -1;
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

    D3DCOLOR ScaledColor(D3DCOLOR color, float extra)
    {
        float const a = ((color >> 24) & 0xFF) / 255.0f * std::clamp(extra, 0.0f, 1.0f);
        return (color & 0x00FFFFFFu) | (static_cast<DWORD>(a * 255.0f + 0.5f) << 24);
    }

    float AdvanceFade(float alpha, float target, float dt, float fadeIn, float fadeOut)
    {
        float const fade = (alpha < target) ? std::max(0.02f, fadeIn) : std::max(0.02f, fadeOut);
        float const step = std::min(0.5f, dt / fade);
        if (alpha < target)
            return std::min(1.0f, alpha + step);
        if (alpha > target)
            return std::max(0.0f, alpha - step);
        return alpha;
    }

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
        if (ll < 0.001f)
            return;
        lx /= ll;
        ly /= ll;

        float px = bl.x - tl.x;
        float py = bl.y - tl.y;
        float const pl = std::sqrt(px * px + py * py);
        if (pl < 0.001f)
            return;
        px /= pl;
        py /= pl;

        float const width = (ll + ScreenDistance(bl, br)) * 0.5f;
        float const height = (pl + ScreenDistance(tr, br)) * 0.5f;
        float const pad = 1.0f;

        ScreenPoint const otl = { tl.x - px * pad - lx * pad, tl.y - py * pad - ly * pad };
        ScreenPoint const otr = { tr.x - px * pad + lx * pad, tr.y - py * pad + ly * pad };
        ScreenPoint const obl = { bl.x + px * pad - lx * pad, bl.y + py * pad - ly * pad };
        ScreenPoint const obr = { br.x + px * pad + lx * pad, br.y + py * pad + ly * pad };

        float const inner = -pad;
        float const outerX = width + pad;
        float const outerY = height + pad;

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
        set(v[0], otl, inner, outerX, inner, outerY);
        set(v[1], otr, outerX, inner, inner, outerY);
        set(v[2], obl, inner, outerX, outerY, inner);
        set(v[3], obr, outerX, inner, outerY, inner);

        out.push_back(v[0]);
        out.push_back(v[1]);
        out.push_back(v[2]);
        out.push_back(v[2]);
        out.push_back(v[1]);
        out.push_back(v[3]);
    }

    template <class ProjectPoint>
    void AppendAABar(std::vector<BarVertex>& out, ProjectPoint const& projectPoint, float x0, float y0, float x1,
        float y1, D3DCOLOR color)
    {
        ScreenPoint tl, tr, bl, br;
        if (!projectPoint(x0, y1, tl) || !projectPoint(x1, y1, tr) || !projectPoint(x0, y0, bl) || !projectPoint(x1, y0, br))
            return;

        AppendSoftRect(out, tl, tr, bl, br, color);
    }

    void AppendSolidTri(std::vector<BarVertex>& out, ScreenPoint const& a, ScreenPoint const& b,
        ScreenPoint const& c, D3DCOLOR color)
    {
        BarVertex v[3];
        ScreenPoint const points[3] = { a, b, c };
        for (int i = 0; i < 3; ++i) {
            v[i].x = points[i].x;
            v[i].y = points[i].y;
            v[i].z = 0.0f;
            v[i].rhw = 1.0f;
            v[i].color = color;
            v[i].u = 64.0f;
            v[i].v = 64.0f;
            v[i].u2 = 64.0f;
            v[i].v2 = 64.0f;
            out.push_back(v[i]);
        }
    }

    template <class ProjectPoint>
    void AppendSolidRect(std::vector<BarVertex>& out, ProjectPoint const& projectPoint, float x0, float y0,
        float x1, float y1, D3DCOLOR color)
    {
        ScreenPoint tl, tr, bl, br;
        if (!projectPoint(x0, y1, tl) || !projectPoint(x1, y1, tr) || !projectPoint(x0, y0, bl) || !projectPoint(x1, y0, br))
            return;
        AppendSolidTri(out, tl, tr, bl, color);
        AppendSolidTri(out, bl, tr, br, color);
    }

    template <class ProjectPoint>
    void AppendRing(std::vector<BarVertex>& out, ProjectPoint const& projectPoint, float cx, float cy, float rx,
        float ry, float innerRatio, D3DCOLOR color)
    {
        constexpr int kSegments = 28;
        for (int i = 0; i < kSegments; ++i) {
            float const a0 = i * 2.0f * kPi / kSegments;
            float const a1 = (i + 1) * 2.0f * kPi / kSegments;
            float const c0 = std::cos(a0), s0 = std::sin(a0);
            float const c1 = std::cos(a1), s1 = std::sin(a1);
            ScreenPoint o0, o1, t0, t1;
            if (!projectPoint(cx + c0 * rx, cy + s0 * ry, o0) || !projectPoint(cx + c1 * rx, cy + s1 * ry, o1) || !projectPoint(cx + c0 * rx * innerRatio, cy + s0 * ry * innerRatio, t0) || !projectPoint(cx + c1 * rx * innerRatio, cy + s1 * ry * innerRatio, t1))
                return;
            AppendSolidTri(out, o0, o1, t0, color);
            AppendSolidTri(out, t0, o1, t1, color);
        }
    }

    template <class ProjectPoint>
    void AppendStar(std::vector<BarVertex>& out, ProjectPoint const& projectPoint, float cx, float cy, float rx,
        float ry, D3DCOLOR color)
    {
        constexpr int kPoints = 5;
        ScreenPoint outer[kPoints], inner[kPoints], center;
        if (!projectPoint(cx, cy, center))
            return;
        for (int i = 0; i < kPoints; ++i) {
            float const ao = -kPi * 0.5f + i * 2.0f * kPi / kPoints;
            float const ai = ao + kPi / kPoints;
            if (!projectPoint(cx + std::cos(ao) * rx, cy + std::sin(ao) * ry, outer[i]) || !projectPoint(cx + std::cos(ai) * rx * 0.4f, cy + std::sin(ai) * ry * 0.4f, inner[i]))
                return;
        }
        for (int i = 0; i < kPoints; ++i) {
            int const j = (i + 1) % kPoints;
            AppendSolidTri(out, center, outer[i], inner[i], color);
            AppendSolidTri(out, center, inner[i], outer[j], color);
        }
    }

    template <class ProjectPoint>
    bool DrawString(IDirect3DDevice9* device, TextRenderer& text, ProjectPoint const& projectPoint,
        std::string const& str, float xLeft, float yCenter, float glyphHeight, D3DCOLOR color)
    {
        int texW = 0;
        int texH = 0;
        IDirect3DTexture9* texture = text.Get(device, str, texW, texH);
        if (!texture || texW <= 0 || texH <= 0)
            return false;

        float const unitAspect = Config::PanelHeight / std::max(Config::PanelLength, 0.0001f);
        float const glyphWidth = glyphHeight * (static_cast<float>(texW) / static_cast<float>(texH)) * unitAspect;

        ScreenPoint tl, tr, bl, br;
        if (!projectPoint(xLeft, yCenter + glyphHeight * 0.5f, tl) || !projectPoint(xLeft + glyphWidth, yCenter + glyphHeight * 0.5f, tr) || !projectPoint(xLeft, yCenter - glyphHeight * 0.5f, bl) || !projectPoint(xLeft + glyphWidth, yCenter - glyphHeight * 0.5f, br))
            return false;

        device->SetTexture(0, texture);
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };
        DrawTexturedQuad(device, quad, uv, color);
        return true;
    }

    template <class ProjectPoint>
    void DrawInfoIcons(IDirect3DDevice9* device, ProjectPoint const& projectPoint, layout::PanelRects const& panels,
        float panelAlpha)
    {
        float const unitAspect = Config::PanelHeight / std::max(Config::PanelLength, 0.0001f);
        D3DCOLOR const color = ScaledColor(Config::PanelHudIconColor, panelAlpha);

        auto iconRadius = [&](layout::Rect const& r) {
            return (r.y1 - r.y0) * Config::PanelHudIconSize * 0.5f;
        };

        std::vector<BarVertex> vertices;
        vertices.reserve(768);

        {
            layout::Rect const& r = panels.resource;
            float const cx = r.x0 + (r.x1 - r.x0) * Config::PanelClockIconX;
            float const cy = (r.y0 + r.y1) * 0.5f;
            float const ry = iconRadius(r);
            float const rx = ry * unitAspect;
            AppendRing(vertices, projectPoint, cx, cy, rx, ry, 0.76f, color);
            AppendSolidRect(vertices, projectPoint, cx - rx * 0.05f, cy, cx + rx * 0.05f, cy + ry * 0.52f, color);
            AppendSolidRect(vertices, projectPoint, cx, cy - ry * 0.05f, cx + rx * 0.44f, cy + ry * 0.05f, color);
        }

        {
            layout::Rect const& r = panels.resource;
            float const cx = r.x0 + (r.x1 - r.x0) * Config::PanelWantedIconX;
            float const cy = (r.y0 + r.y1) * 0.5f;
            float const ry = iconRadius(r);
            AppendStar(vertices, projectPoint, cx, cy, ry * unitAspect, ry, color);
        }

        if (!vertices.empty())
            device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3),
                vertices.data(), sizeof(BarVertex));
    }

    template <class ProjectPoint>
    void DrawInfoText(IDirect3DDevice9* device, TextRenderer& text, ProjectPoint const& projectPoint,
        layout::PanelRects const& panels, float panelAlpha)
    {
        D3DCOLOR const color = ScaledColor(Config::PanelHudTextColor, panelAlpha);
        CPlayerInfo& info = CWorld::Players[CWorld::PlayerInFocus];

        {
            layout::Rect const& r = panels.skill;
            float const cy = (r.y0 + r.y1) * 0.5f;
            float const textHeight = (r.y1 - r.y0) * Config::PanelHudTextSize;
            float const iconHeight = (r.y1 - r.y0) * Config::PanelHudIconSize;
            char buffer[32] { };
            std::snprintf(buffer, sizeof(buffer), "%0*d", std::max(1, Config::PanelMoneyDigits),
                info.m_nMoney);
            DrawString(device, text, projectPoint, "$", r.x0 + (r.x1 - r.x0) * Config::PanelSkillIconX, cy,
                iconHeight, color);
            DrawString(device, text, projectPoint, buffer, r.x0 + (r.x1 - r.x0) * Config::PanelSkillValueX, cy,
                textHeight, color);
        }

        {
            layout::Rect const& r = panels.resource;
            float const cy = (r.y0 + r.y1) * 0.5f;
            float const textHeight = (r.y1 - r.y0) * Config::PanelHudTextSize;
            char buffer[16] { };
            std::snprintf(buffer, sizeof(buffer), "%02d:%02d", CClock::ms_nGameClockHours,
                CClock::ms_nGameClockMinutes);
            DrawString(device, text, projectPoint, buffer, r.x0 + (r.x1 - r.x0) * Config::PanelClockValueX, cy,
                textHeight, color);
        }

        {
            layout::Rect const& r = panels.resource;
            float const cy = (r.y0 + r.y1) * 0.5f;
            float const textHeight = (r.y1 - r.y0) * Config::PanelHudTextSize;
            unsigned int wanted = 0;
            if (info.m_PlayerData.m_pWanted)
                wanted = info.m_PlayerData.m_pWanted->m_nWantedLevel;
            char buffer[16] { };
            std::snprintf(buffer, sizeof(buffer), "%u", wanted);
            DrawString(device, text, projectPoint, buffer, r.x0 + (r.x1 - r.x0) * Config::PanelWantedValueX, cy,
                textHeight, color);
        }
    }

    template <class ProjectPoint>
    void DrawVitals(IDirect3DDevice9* device, ProjectPoint const& projectPoint, float panelAlpha,
        float staminaAlpha, float breathAlpha)
    {
        CPed* ped = FindPlayerPed(0);
        if (!ped)
            return;

        CPlayerInfo& info = CWorld::Players[CWorld::PlayerInFocus];
        float const maxHealth = static_cast<float>(info.m_nMaxHealth);
        float const maxArmour = static_cast<float>(info.m_nMaxArmour);
        float const maxStamina = CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN);
        float const maxAir = CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG);

        float const length = std::clamp(Config::PanelVitalsLength, 0.05f, 1.0f);
        float const x0 = (1.0f - length) * 0.5f;

        float const unit = std::max(0.0001f, Config::PanelVitalsArmorUnit);
        int armorTotal = (maxArmour > 0.0f) ? static_cast<int>(std::lround(maxArmour / unit)) : 1;
        if (armorTotal < 1)
            armorTotal = 1;
        int armorFilled = (maxArmour > 0.0f) ? static_cast<int>(std::lround(ped->m_fArmour / unit)) : 0;
        armorFilled = std::clamp(armorFilled, 0, armorTotal);
        bool const armorPresent = (maxArmour > 0.0f) && (armorFilled > 0);

        float const gap = Config::PanelVitalsGap;
        float const healthBottom = 1.0f + Config::PanelVitalsOffsetY;
        float const healthTop = healthBottom + Config::PanelVitalsThicknessHealth;
        float const armorBottom = healthTop + gap;
        float const armorTop = armorBottom + Config::PanelVitalsThicknessArmor;
        float const staminaBottom = armorPresent ? (armorTop + gap) : armorBottom;
        float const staminaTop = staminaBottom + Config::PanelVitalsThicknessStamina;
        float const breathBottom = staminaTop + gap;
        float const breathTop = breathBottom + Config::PanelVitalsThicknessStamina;

        D3DCOLOR const healthColor = ScaledColor(Config::PanelVitalsHealthColor, panelAlpha);
        D3DCOLOR const trackColor = ScaledColor(Config::PanelVitalsHealthTrackColor, panelAlpha);
        D3DCOLOR const armorColor = ScaledColor(Config::PanelVitalsArmorColor, panelAlpha);
        D3DCOLOR const staminaColor = ScaledColor(Config::PanelVitalsStaminaColor, panelAlpha * staminaAlpha);
        D3DCOLOR const breathColor = ScaledColor(Config::PanelVitalsBreathColor, panelAlpha * breathAlpha);
        D3DCOLOR const cursorColor = ScaledColor(Config::PanelVitalsCursorColor, panelAlpha);
        D3DCOLOR const staminaCursorColor = ScaledColor(Config::PanelVitalsCursorColor, panelAlpha * staminaAlpha);
        D3DCOLOR const breathCursorColor = ScaledColor(Config::PanelVitalsCursorColor, panelAlpha * breathAlpha);

        std::vector<BarVertex> vertices;
        vertices.reserve(640);

        auto addBar = [&](float yBottom, float yTop, float frac, D3DCOLOR color) {
            if (frac <= 0.0f)
                return;
            float const right = x0 + length * std::clamp(frac, 0.0f, 1.0f);
            AppendAABar(vertices, projectPoint, x0, yBottom, right, yTop, color);
        };

        auto addCursor = [&](float yBottom, float yTop, float frac, D3DCOLOR color) {
            float const centerX = x0 + length * std::clamp(frac, 0.0f, 1.0f);
            float const halfW = Config::PanelVitalsCursorWidth * 0.5f;
            float const centerY = (yBottom + yTop) * 0.5f;
            float const halfH = (yTop - yBottom) * 0.5f * Config::PanelVitalsCursorScale;
            AppendAABar(vertices, projectPoint, centerX - halfW, centerY - halfH, centerX + halfW,
                centerY + halfH, color);
        };

        float const healthFrac = (maxHealth > 0.0f) ? ped->m_fHealth / maxHealth : 0.0f;
        if (maxHealth > 0.0f) {
            addBar(healthBottom, healthTop, 1.0f, trackColor);
            addBar(healthBottom, healthTop, healthFrac, healthColor);
            addCursor(healthBottom, healthTop, healthFrac, cursorColor);
        }

        if (armorPresent) {
            float const segmentWidth = length / static_cast<float>(armorTotal);
            float const segmentGap = Config::PanelVitalsArmorGap;
            for (int i = 0; i < armorFilled; ++i) {
                float const segX0 = x0 + segmentWidth * i + segmentGap * 0.5f;
                float const segX1 = x0 + segmentWidth * (i + 1) - segmentGap * 0.5f;
                if (segX1 <= segX0)
                    continue;
                AppendAABar(vertices, projectPoint, segX0, armorBottom, segX1, armorTop, armorColor);
            }
        }

        if (maxStamina > 0.0f && staminaAlpha > 0.001f && ped->m_pPlayerData) {
            float const staminaFrac = std::clamp(
                (ped->m_pPlayerData->m_fTimeCanRun - kStaminaFloor) / (maxStamina - kStaminaFloor), 0.0f, 1.0f);
            addBar(staminaBottom, staminaTop, staminaFrac, staminaColor);
            addCursor(staminaBottom, staminaTop, staminaFrac, staminaCursorColor);
        }

        if (maxAir > 0.0f && breathAlpha > 0.001f && ped->m_pPlayerData) {
            float const breathFrac = std::clamp(ped->m_pPlayerData->m_fBreath / maxAir, 0.0f, 1.0f);
            addBar(breathBottom, breathTop, breathFrac, breathColor);
            addCursor(breathBottom, breathTop, breathFrac, breathCursorColor);
        }

        if (!vertices.empty())
            device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3),
                vertices.data(), sizeof(BarVertex));
    }

    IDirect3DPixelShader9* CreateBarShader(IDirect3DDevice9* device)
    {
        char const* source = "float4 main(float2 a : TEXCOORD0, float2 b : TEXCOORD1, float4 color : COLOR0) : COLOR\n"
                             "{\n"
                             "    float d = min(min(a.x, a.y), min(b.x, b.y));\n"
                             "    float cov = saturate(d + 0.5);\n"
                             "    return float4(color.rgb, color.a * cov);\n"
                             "}\n";

        ID3DBlob* code = nullptr;
        ID3DBlob* errors = nullptr;
        HRESULT const hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "main", "ps_2_0",
            0, 0, &code, &errors);
        if (errors)
            errors->Release();
        if (FAILED(hr) || !code)
            return nullptr;

        IDirect3DPixelShader9* shader = nullptr;
        device->CreatePixelShader(reinterpret_cast<DWORD const*>(code->GetBufferPointer()), &shader);
        code->Release();
        return shader;
    }

    IDirect3DPixelShader9* CreateLineShader(IDirect3DDevice9* device)
    {
        char const* source = "float4 main(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR\n"
                             "{\n"
                             "    float d = abs(uv.x);\n"
                             "    float a = saturate(uv.y + 0.5 - d);\n"
                             "    return float4(color.rgb, color.a * a);\n"
                             "}\n";

        ID3DBlob* code = nullptr;
        ID3DBlob* errors = nullptr;
        HRESULT const hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "main", "ps_2_0",
            0, 0, &code, &errors);
        if (errors)
            errors->Release();
        if (FAILED(hr) || !code)
            return nullptr;

        IDirect3DPixelShader9* shader = nullptr;
        device->CreatePixelShader(reinterpret_cast<DWORD const*>(code->GetBufferPointer()), &shader);
        code->Release();
        return shader;
    }

    void DrawLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to, float thickness,
        D3DCOLOR color)
    {
        float const dx = to.x - from.x;
        float const dy = to.y - from.y;
        float const length = std::sqrt(dx * dx + dy * dy);
        if (length < 0.001f)
            return;

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
        if (length < 0.001f)
            return;

        float nx = -dy / length;
        float ny = dx / length;
        if (nx * (interior.x - from.x) + ny * (interior.y - from.y) < 0.0f) {
            nx = -nx;
            ny = -ny;
        }

        float const offset = thickness * 0.5f + 0.5f;
        ScreenPoint const shiftedFrom = { from.x + nx * offset, from.y + ny * offset };
        ScreenPoint const shiftedTo = { to.x + nx * offset, to.y + ny * offset };
        DrawLine(device, shiftedFrom, shiftedTo, thickness, color);
    }

    class DeviceStateGuard
    {
    public:
        explicit DeviceStateGuard(IDirect3DDevice9* device)
            : m_device(device)
        { Save(); }
        ~DeviceStateGuard() { Restore(); }

        DeviceStateGuard(DeviceStateGuard const&) = delete;
        DeviceStateGuard& operator=(DeviceStateGuard const&) = delete;

    private:
        void Save()
        {
            RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &m_rwTextureRaster);
            RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &m_rwZTest);
            RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &m_rwZWrite);
            RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &m_rwVertexAlpha);
            RwRenderStateGet(rwRENDERSTATEFOGENABLE, &m_rwFog);
            RwRenderStateGet(rwRENDERSTATESRCBLEND, &m_rwSrcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, &m_rwDestBlend);
            RwRenderStateGet(rwRENDERSTATECULLMODE, &m_rwCullMode);

            m_device->GetVertexShader(&m_vertexShader);
            m_device->GetPixelShader(&m_pixelShader);
            m_device->GetVertexDeclaration(&m_vertexDeclaration);
            m_device->GetFVF(&m_fvf);
            m_device->GetTexture(0, &m_texture);
            m_device->GetStreamSource(0, &m_stream, &m_streamOffset, &m_streamStride);

            m_device->GetRenderState(D3DRS_ZENABLE, &m_renderStates[0]);
            m_device->GetRenderState(D3DRS_ZWRITEENABLE, &m_renderStates[1]);
            m_device->GetRenderState(D3DRS_CULLMODE, &m_renderStates[2]);
            m_device->GetRenderState(D3DRS_LIGHTING, &m_renderStates[3]);
            m_device->GetRenderState(D3DRS_FOGENABLE, &m_renderStates[4]);
            m_device->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_renderStates[5]);
            m_device->GetRenderState(D3DRS_SRCBLEND, &m_renderStates[6]);
            m_device->GetRenderState(D3DRS_DESTBLEND, &m_renderStates[7]);
            m_device->GetRenderState(D3DRS_ALPHATESTENABLE, &m_renderStates[8]);
            m_device->GetRenderState(D3DRS_SCISSORTESTENABLE, &m_renderStates[9]);

            m_device->GetTextureStageState(0, D3DTSS_COLOROP, &m_textureStages[0]);
            m_device->GetTextureStageState(0, D3DTSS_COLORARG1, &m_textureStages[1]);
            m_device->GetTextureStageState(0, D3DTSS_COLORARG2, &m_textureStages[2]);
            m_device->GetTextureStageState(0, D3DTSS_ALPHAOP, &m_textureStages[3]);
            m_device->GetTextureStageState(0, D3DTSS_ALPHAARG1, &m_textureStages[4]);
            m_device->GetTextureStageState(0, D3DTSS_ALPHAARG2, &m_textureStages[5]);

            m_device->GetSamplerState(0, D3DSAMP_MINFILTER, &m_samplerStates[0]);
            m_device->GetSamplerState(0, D3DSAMP_MAGFILTER, &m_samplerStates[1]);
            m_device->GetSamplerState(0, D3DSAMP_MIPFILTER, &m_samplerStates[2]);
            m_device->GetSamplerState(0, D3DSAMP_ADDRESSU, &m_samplerStates[3]);
            m_device->GetSamplerState(0, D3DSAMP_ADDRESSV, &m_samplerStates[4]);
        }

        void Restore()
        {
            m_device->SetVertexShader(m_vertexShader);
            m_device->SetPixelShader(m_pixelShader);
            if (m_vertexDeclaration)
                m_device->SetVertexDeclaration(m_vertexDeclaration);
            else
                m_device->SetFVF(m_fvf);
            m_device->SetTexture(0, m_texture);
            m_device->SetStreamSource(0, m_stream, m_streamOffset, m_streamStride);

            m_device->SetRenderState(D3DRS_ZENABLE, m_renderStates[0]);
            m_device->SetRenderState(D3DRS_ZWRITEENABLE, m_renderStates[1]);
            m_device->SetRenderState(D3DRS_CULLMODE, m_renderStates[2]);
            m_device->SetRenderState(D3DRS_LIGHTING, m_renderStates[3]);
            m_device->SetRenderState(D3DRS_FOGENABLE, m_renderStates[4]);
            m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, m_renderStates[5]);
            m_device->SetRenderState(D3DRS_SRCBLEND, m_renderStates[6]);
            m_device->SetRenderState(D3DRS_DESTBLEND, m_renderStates[7]);
            m_device->SetRenderState(D3DRS_ALPHATESTENABLE, m_renderStates[8]);
            m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, m_renderStates[9]);

            m_device->SetTextureStageState(0, D3DTSS_COLOROP, m_textureStages[0]);
            m_device->SetTextureStageState(0, D3DTSS_COLORARG1, m_textureStages[1]);
            m_device->SetTextureStageState(0, D3DTSS_COLORARG2, m_textureStages[2]);
            m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, m_textureStages[3]);
            m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, m_textureStages[4]);
            m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, m_textureStages[5]);

            m_device->SetSamplerState(0, D3DSAMP_MINFILTER, m_samplerStates[0]);
            m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, m_samplerStates[1]);
            m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, m_samplerStates[2]);
            m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, m_samplerStates[3]);
            m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, m_samplerStates[4]);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, m_rwTextureRaster);
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, m_rwZTest);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, m_rwZWrite);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, m_rwVertexAlpha);
            RwRenderStateSet(rwRENDERSTATEFOGENABLE, m_rwFog);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, m_rwSrcBlend);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, m_rwDestBlend);
            RwRenderStateSet(rwRENDERSTATECULLMODE, m_rwCullMode);

            if (m_vertexShader) m_vertexShader->Release();
            if (m_pixelShader) m_pixelShader->Release();
            if (m_vertexDeclaration) m_vertexDeclaration->Release();
            if (m_texture) m_texture->Release();
            if (m_stream) m_stream->Release();
        }

        IDirect3DDevice9* m_device;

        void* m_rwTextureRaster = nullptr;
        void* m_rwZTest = nullptr;
        void* m_rwZWrite = nullptr;
        void* m_rwVertexAlpha = nullptr;
        void* m_rwFog = nullptr;
        void* m_rwSrcBlend = nullptr;
        void* m_rwDestBlend = nullptr;
        void* m_rwCullMode = nullptr;

        IDirect3DVertexShader9* m_vertexShader = nullptr;
        IDirect3DPixelShader9* m_pixelShader = nullptr;
        IDirect3DVertexDeclaration9* m_vertexDeclaration = nullptr;
        DWORD m_fvf = 0;
        IDirect3DBaseTexture9* m_texture = nullptr;
        IDirect3DVertexBuffer9* m_stream = nullptr;
        UINT m_streamOffset = 0;
        UINT m_streamStride = 0;

        DWORD m_renderStates[10] { };
        DWORD m_textureStages[6] { };
        DWORD m_samplerStates[5] { };
    };
}

void Panel3D::Update()
{
    Config::Update();

    CPed* ped = FindPlayerPed(0);
    bool const inVehicle = FindPlayerVehicle(0, false) != nullptr;

    float dt = CTimer::ms_fTimeStep * kTimeStepToSeconds;
    if (!(dt > 0.0f))
        dt = 1.0f / 60.0f;

    bool aiming = false;
    if (ped && !inVehicle) {
        bool const pedAimingGun = ped->bIsAimingGun;
        ePedState const pedState = ped->m_ePedState;
        unsigned char const activeCam = TheCamera.m_nActiveCam;
        eCamMode const mode = TheCamera.m_aCams[activeCam < 3 ? activeCam : 0].m_nMode;
        bool const camAiming = mode == MODE_AIMWEAPON || mode == MODE_AIMWEAPON_FROMCAR || mode == MODE_AIMWEAPON_ATTACHED || mode == MODE_SNIPER || mode == MODE_ROCKETLAUNCHER || mode == MODE_1STPERSON || mode == MODE_M16_1STPERSON || mode == MODE_SNIPER_RUNABOUT || mode == MODE_ROCKETLAUNCHER_RUNABOUT || mode == MODE_1STPERSON_RUNABOUT || mode == MODE_M16_1STPERSON_RUNABOUT || mode == MODE_ROCKETLAUNCHER_HS || mode == MODE_ROCKETLAUNCHER_RUNABOUT_HS || mode == MODE_HELICANNON_1STPERSON;

        aiming = camAiming || pedAimingGun || pedState == PEDSTATE_AIMGUN || pedState == PEDSTATE_SNIPER_MODE || pedState == PEDSTATE_ROCKETLAUNCHER_MODE;
    }

    m_aiming = aiming;

    float const adsTarget = aiming ? 1.0f : 0.0f;
    float const adsStep = dt / std::max(0.01f, Config::PanelAdsDuration);
    if (m_adsBlend < adsTarget)
        m_adsBlend = std::min(adsTarget, m_adsBlend + adsStep);
    else
        m_adsBlend = std::max(adsTarget, m_adsBlend - adsStep);

    if (ped) {
        CVector rawSide = CameraRight();
        rawSide.z = 0.0f;
        if (rawSide.MagnitudeSqr() < 1e-6f)
            rawSide = CVector(1.0f, 0.0f, 0.0f);
        rawSide.Normalize();

        if (!m_sideInitialised) {
            m_side = rawSide;
            m_sideInitialised = true;
        } else {
            m_side += (rawSide - m_side) * std::min(1.0f, dt * Config::SideSmooth);
            m_side.Normalize();
        }

        CVector rawForward = CameraForward();
        rawForward.z = 0.0f;
        if (rawForward.MagnitudeSqr() < 1e-6f)
            rawForward = CVector(0.0f, 1.0f, 0.0f);
        rawForward.Normalize();

        if (!m_forwardInitialised) {
            m_forward = rawForward;
            m_forwardInitialised = true;
        } else {
            m_forward += (rawForward - m_forward) * std::min(1.0f, dt * Config::SideSmooth);
            m_forward.Normalize();
        }

        CVector base = ped->GetPosition();
        if (Config::BoneInfluence > 0.0f && ped->m_pRwClump) {
            RwV3d bone { };
            ped->GetBonePosition(bone, BONE_UPPERTORSO, false);
            CVector const boneOffset = CVector(bone) - base;
            if (!m_boneNeutralSet) {
                m_boneNeutral = boneOffset;
                m_boneNeutralSet = true;
            }
            base += (boneOffset - m_boneNeutral) * Config::BoneInfluence;
        }

        CVector target = base;
        target += m_side * Config::AnchorOffsetRight;
        target += m_forward * Config::AnchorOffsetForward;
        target += CVector(0.0f, 0.0f, 1.0f) * Config::AnchorOffsetUp;

        if (!m_targetInitialised) {
            m_target = target;
            m_targetInitialised = true;
        } else {
            m_target += (target - m_target) * std::min(1.0f, dt * Config::AnchorSmooth);
        }

        if (!m_initialised) {
            m_position = m_target;
            m_velocity = CVector(0.0f);
            m_initialised = true;
        } else {
            Damp(m_position, m_velocity, m_target, Config::SpringOmega, dt);
        }

        float const heading = ped->GetHeading();
        float turn = 0.0f;
        if (m_hasPrevHeading)
            turn = std::clamp(WrapAngle(heading - m_prevHeading) / dt, -400.0f, 400.0f);
        m_prevHeading = heading;
        m_hasPrevHeading = true;

        float const targetRoll = std::clamp(-turn * Config::TurnTiltGain, -Config::TurnTiltMax, Config::TurnTiltMax);
        m_roll += (targetRoll - m_roll) * std::min(1.0f, dt * 450.0f);

        CVector forward = CameraForward();
        forward.Normalize();

        float const rawYaw = std::atan2(forward.y, forward.x);
        if (!m_yawInitialised) {
            m_yaw = rawYaw;
            m_yawInitialised = true;
        } else {
            m_yaw += WrapAngle(rawYaw - m_yaw) * std::min(1.0f, dt * Config::YawSmooth);
        }

        float const cameraPitch = std::asin(std::clamp(forward.z, -1.0f, 1.0f));
        float const excess = std::max(0.0f, std::fabs(cameraPitch) - Config::PitchDeadzone);
        float const targetPitch = Config::BasePitch - std::copysign(excess, cameraPitch);

        if (!m_pitchInitialised) {
            m_pitch = targetPitch;
            m_pitchInitialised = true;
        } else {
            m_pitch += (targetPitch - m_pitch) * std::min(1.0f, dt * Config::PitchSmooth);
        }

        m_bobPhase += dt * Config::BobOmega;
    }

    float const targetAlpha = (ped && !inVehicle) ? 1.0f : 0.0f;
    float const rate = (targetAlpha > m_alpha) ? Config::FadeInSpeed : Config::FadeOutSpeed;
    m_alpha += (targetAlpha - m_alpha) * std::min(1.0f, dt * rate);

    if (ped && ped->m_pPlayerData) {
        float const maxStamina = CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN);
        float const stamina = ped->m_pPlayerData->m_fTimeCanRun;
        float const epsilon = std::max(0.5f, maxStamina * 0.005f);
        bool const notFull = (maxStamina > 0.0f) && (stamina < maxStamina - epsilon);

        if (notFull)
            m_staminaHold = Config::PanelVitalsStaminaHold;
        else
            m_staminaHold = std::max(0.0f, m_staminaHold - dt);

        float const staminaTarget = (m_staminaHold > 0.0f) ? 1.0f : 0.0f;
        m_staminaAlpha = AdvanceFade(m_staminaAlpha, staminaTarget, dt, Config::PanelVitalsStaminaFadeIn,
            Config::PanelVitalsStaminaFadeOut);
    }

    if (ped && ped->m_pPlayerData && ped->m_pIntelligence) {
        float const maxAir = CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG);
        bool active = false;
        if (maxAir > 0.0f) {
            bool const swimming = ped->m_pIntelligence->GetTaskSwim() != nullptr;
            active = swimming || (ped->m_pPlayerData->m_fBreath < maxAir);
        }

        if (active)
            m_breathHold = Config::PanelVitalsBreathHold;
        else
            m_breathHold = std::max(0.0f, m_breathHold - dt);

        float const breathTarget = (m_breathHold > 0.0f) ? 1.0f : 0.0f;
        m_breathAlpha = AdvanceFade(m_breathAlpha, breathTarget, dt, Config::PanelVitalsStaminaFadeIn,
            Config::PanelVitalsStaminaFadeOut);
    }
}

void Panel3D::Draw()
{
    if (m_alpha < 0.01f || !m_initialised)
        return;

    CPed* ped = FindPlayerPed(0);
    if (!ped || FindPlayerVehicle(0, false))
        return;

    IDirect3DDevice9* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    if (!device)
        return;

    float alpha = m_alpha;
    {
        CVector const& cameraPosition = TheCamera.GetPosition();
        float const distance = CVector::Distance(m_position, cameraPosition);
        float nearFade = 1.0f;
        if (distance < Config::FadeNearDist)
            nearFade = 0.0f;
        else if (distance < Config::FadeNearFull)
            nearFade = (distance - Config::FadeNearDist) / (Config::FadeNearFull - Config::FadeNearDist);
        nearFade += (1.0f - nearFade) * m_adsBlend;
        if (nearFade < 0.01f)
            return;
        alpha *= nearFade;
    }

    int const screenWidth = RsGlobal.maximumWidth;
    int const screenHeight = RsGlobal.maximumHeight;
    if (screenWidth <= 0 || screenHeight <= 0)
        return;

    float const yaw = m_yaw + kPi + Config::BaseYawOffset;
    float const pitch = m_pitch;

    CVector normal(std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch));
    normal.Normalize();

    CVector const worldUp(0.0f, 0.0f, 1.0f);
    CVector right = worldUp.Cross(normal);
    if (right.MagnitudeSqr() < 1e-6f)
        right = CVector(1.0f, 0.0f, 0.0f);
    right.Normalize();
    CVector up = normal.Cross(right);
    up.Normalize();

    {
        CVector const rolledRight = right * std::cos(m_roll) + up * std::sin(m_roll);
        CVector const rolledUp = up * std::cos(m_roll) - right * std::sin(m_roll);
        right = rolledRight;
        up = rolledUp;
    }

    CVector center = m_position;
    center.z += std::sin(m_bobPhase) * Config::BobAmplitude;

    layout::PanelRects const panels = layout::Compute(Config::PanelColumnLength, Config::PanelSkillHeight, Config::PanelGap);

    float const screenW = static_cast<float>(screenWidth);
    float const screenH = static_cast<float>(screenHeight);

    auto toWorld = [&](float nx, float ny) {
        CVector point = center;
        point += right * ((nx - 0.5f) * Config::PanelLength);
        point += up * ((ny - 0.5f) * Config::PanelHeight);
        return point;
    };

    float const adsWidth = screenW * Config::PanelAdsWidth;
    float const adsHeight = adsWidth * (Config::PanelHeight / std::max(Config::PanelLength, 0.0001f));
    float const adsCenterX = screenW * Config::PanelAdsCenterX;
    float const adsCenterY = screenH * Config::PanelAdsCenterY;

    float const cornerLocal[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    ScreenPoint worldQuad[4];
    for (int i = 0; i < 4; ++i) {
        if (!Project(toWorld(cornerLocal[i][0], cornerLocal[i][1]), worldQuad[i])) {
            worldQuad[i].x = adsCenterX;
            worldQuad[i].y = adsCenterY;
        }
    }

    float const depth = adsWidth * 2.0f;
    float const cp = std::cos(Config::PanelAdsPitch);
    float const sp = std::sin(Config::PanelAdsPitch);
    float const cyaw = std::cos(Config::PanelAdsYaw);
    float const syaw = std::sin(Config::PanelAdsYaw);
    ScreenPoint adsQuad[4];
    for (int i = 0; i < 4; ++i) {
        float const x = (cornerLocal[i][0] - 0.5f) * adsWidth;
        float const y = (cornerLocal[i][1] - 0.5f) * adsHeight;

        float const yawX = x * cyaw;
        float const yawZ = -x * syaw;

        float const pitchY = y * cp + yawZ * sp;
        float const pitchZ = -y * sp + yawZ * cp;

        float const perspective = depth / (depth - pitchZ);
        adsQuad[i].x = adsCenterX + yawX * perspective;
        adsQuad[i].y = adsCenterY - pitchY * perspective;
    }

    if (!m_aiming) {
        for (int i = 0; i < 4; ++i) {
            m_adsFrom[i][0] = worldQuad[i].x;
            m_adsFrom[i][1] = worldQuad[i].y;
        }
    }

    ScreenPoint quad[4];
    for (int i = 0; i < 4; ++i) {
        quad[i].x = m_adsFrom[i][0] + (adsQuad[i].x - m_adsFrom[i][0]) * m_adsBlend;
        quad[i].y = m_adsFrom[i][1] + (adsQuad[i].y - m_adsFrom[i][1]) * m_adsBlend;
    }

    auto projectPoint = [&](float nx, float ny, ScreenPoint& out) {
        float const bottomX = quad[0].x + (quad[1].x - quad[0].x) * nx;
        float const bottomY = quad[0].y + (quad[1].y - quad[0].y) * nx;
        float const topX = quad[3].x + (quad[2].x - quad[3].x) * nx;
        float const topY = quad[3].y + (quad[2].y - quad[3].y) * nx;
        out.x = bottomX + (topX - bottomX) * ny;
        out.y = bottomY + (topY - bottomY) * ny;
        return true;
    };

    auto projectRect = [&](layout::Rect const& rect, ScreenPoint(&out)[4]) {
        return projectPoint(rect.x0, rect.y1, out[0]) && projectPoint(rect.x1, rect.y1, out[1]) && projectPoint(rect.x0, rect.y0, out[2]) && projectPoint(rect.x1, rect.y0, out[3]);
    };

    ScreenPoint ammo[4], skill[4], resource[4];
    if (!projectRect(panels.ammo, ammo) || !projectRect(panels.skill, skill) || !projectRect(panels.resource, resource))
        return;

    layout::Rect const& ammoPanel = panels.ammo;
    float const ammoW = ammoPanel.x1 - ammoPanel.x0;
    float const ammoH = ammoPanel.y1 - ammoPanel.y0;
    float const splitRatio = std::clamp(Config::PanelAmmoSplit, 0.05f, 0.95f);
    float const dividerX = ammoPanel.x0 + ammoW * splitRatio;

    layout::Rect const leftRegion = { ammoPanel.x0, ammoPanel.y0, dividerX, ammoPanel.y1 };
    layout::Rect const rightRegion = { dividerX, ammoPanel.y0, ammoPanel.x1, ammoPanel.y1 };

    float const clipSplit = std::clamp(Config::PanelAmmoClipSplit, 0.1f, 0.9f);
    float const leftHeight = leftRegion.y1 - leftRegion.y0;
    float const clipZoneY0 = leftRegion.y1 - leftHeight * clipSplit;
    layout::Rect const clipZone = { leftRegion.x0, clipZoneY0, leftRegion.x1, leftRegion.y1 };
    layout::Rect const reserveZone = { leftRegion.x0, leftRegion.y0, leftRegion.x1, clipZoneY0 };

    float const rightMid = (rightRegion.y0 + rightRegion.y1) * 0.5f;
    layout::Rect const prevZone = { rightRegion.x0, rightMid, rightRegion.x1, rightRegion.y1 };
    layout::Rect const nextZone = { rightRegion.x0, rightRegion.y0, rightRegion.x1, rightMid };

    int const currentSlot = ped->m_nSelectedWepSlot;
    CWeapon const& currentWeapon = ped->m_aWeapons[currentSlot];
    int const prevSlot = FindWeaponSlot(ped, currentSlot, -1);
    int const nextSlot = FindWeaponSlot(ped, currentSlot, 1);

    DeviceStateGuard guard(device);

    IDirect3DTexture9* blurred = m_blur.Update(device);
    if (!blurred)
        return;

    device->SetVertexShader(nullptr);
    device->SetPixelShader(nullptr);
    device->SetVertexDeclaration(nullptr);
    device->SetFVF(kFvf);
    device->SetStreamSource(0, nullptr, 0, 0);
    device->SetTexture(0, nullptr);

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

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

    float const tintR = ((Config::BackTint >> 16) & 0xFF) / 255.0f;
    float const tintG = ((Config::BackTint >> 8) & 0xFF) / 255.0f;
    float const tintB = (Config::BackTint & 0xFF) / 255.0f;
    float const tintA = (((Config::BackTint >> 24) & 0xFF) / 255.0f) * alpha;
    D3DCOLOR const tint = D3DCOLOR_COLORVALUE(tintR, tintG, tintB, tintA);

    auto drawBackdrop = [&](ScreenPoint const(&points)[4]) {
        float uv[4][2];
        for (int i = 0; i < 4; ++i) {
            uv[i][0] = points[i].x / screenW;
            uv[i][1] = points[i].y / screenH;
        }
        DrawTexturedQuad(device, points, uv, tint);
    };

    device->SetTexture(0, blurred);
    drawBackdrop(ammo);
    drawBackdrop(skill);
    drawBackdrop(resource);

    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    D3DCOLOR const lineColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, alpha * Config::PanelLineAlpha);
    D3DCOLOR const bottomColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, alpha * Config::PanelLineAlphaBottom);
    float const lineThickness = Config::PanelLineWidth;

    if (!m_lineShader)
        m_lineShader = CreateLineShader(device);
    device->SetPixelShader(m_lineShader);
    device->SetTexture(0, nullptr);

    ScreenPoint ammoCenter, skillCenter, resourceCenter;
    projectPoint((panels.ammo.x0 + panels.ammo.x1) * 0.5f, (panels.ammo.y0 + panels.ammo.y1) * 0.5f, ammoCenter);
    projectPoint((panels.skill.x0 + panels.skill.x1) * 0.5f, (panels.skill.y0 + panels.skill.y1) * 0.5f, skillCenter);
    projectPoint((panels.resource.x0 + panels.resource.x1) * 0.5f, (panels.resource.y0 + panels.resource.y1) * 0.5f, resourceCenter);

    auto drawEdge = [&](layout::Rect const& rect, bool top, ScreenPoint const& interior, D3DCOLOR color) {
        float const y = top ? rect.y1 : rect.y0;
        ScreenPoint a, b;
        if (projectPoint(rect.x0, y, a) && projectPoint(rect.x1, y, b))
            DrawEdgeLine(device, a, b, interior, lineThickness, color);
    };

    drawEdge(panels.ammo, true, ammoCenter, lineColor);
    drawEdge(panels.ammo, false, ammoCenter, lineColor);
    drawEdge(panels.skill, true, skillCenter, lineColor);
    drawEdge(panels.skill, false, skillCenter, bottomColor);
    drawEdge(panels.resource, true, resourceCenter, lineColor);
    drawEdge(panels.resource, false, resourceCenter, bottomColor);

    {
        ScreenPoint dividerTop, dividerBottom;
        if (projectPoint(dividerX, ammoPanel.y1, dividerTop) && projectPoint(dividerX, ammoPanel.y0, dividerBottom))
            DrawLine(device, dividerTop, dividerBottom, lineThickness, lineColor);
    }

    if (!m_barShader)
        m_barShader = CreateBarShader(device);
    device->SetPixelShader(m_barShader);
    device->SetFVF(kBarFvf);
    DrawVitals(device, projectPoint, alpha, m_staminaAlpha, m_breathAlpha);
    DrawInfoIcons(device, projectPoint, panels, alpha);
    device->SetFVF(kFvf);

    if (!m_textInitialised) {
        m_text.Initialize(PLUGIN_PATH("Borda Medium.ttf"), 128);
        m_textInitialised = true;
    }

    device->SetPixelShader(nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    auto drawNumber = [&](layout::Rect const& zone, int value, float sizeFraction, float offsetX, float offsetY,
                          D3DCOLOR color) {
        std::string const text = std::to_string(value);
        int texW = 0;
        int texH = 0;
        IDirect3DTexture9* texture = m_text.Get(device, text, texW, texH);
        if (!texture || texW <= 0 || texH <= 0)
            return;

        float const zoneW = zone.x1 - zone.x0;
        float const zoneH = zone.y1 - zone.y0;
        if (zoneW <= 0.0f || zoneH <= 0.0f)
            return;

        float const unitAspect = Config::PanelHeight / std::max(Config::PanelLength, 0.0001f);
        float const textureAspect = (static_cast<float>(texW) / static_cast<float>(texH)) * unitAspect;

        float glyphHeight = zoneH * sizeFraction;
        float glyphWidth = glyphHeight * textureAspect;
        if (glyphWidth > zoneW) {
            glyphWidth = zoneW;
            glyphHeight = glyphWidth / textureAspect;
        }

        float const xL = zone.x0 + (zoneW - glyphWidth) * 0.5f + offsetX;
        float const xR = xL + glyphWidth;
        float const yCenter = (zone.y0 + zone.y1) * 0.5f + offsetY;
        float const yTop = yCenter + glyphHeight * 0.5f;
        float const yBottom = yCenter - glyphHeight * 0.5f;

        ScreenPoint tl, tr, bl, br;
        if (!projectPoint(xL, yTop, tl) || !projectPoint(xR, yTop, tr) || !projectPoint(xL, yBottom, bl) || !projectPoint(xR, yBottom, br))
            return;

        device->SetTexture(0, texture);
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };

        if (Config::PanelAmmoShadowOffset > 0.0f) {
            float const so = Config::PanelAmmoShadowOffset;
            ScreenPoint const shadowQuad[4] = {
                { tl.x + so, tl.y + so },
                { tr.x + so, tr.y + so },
                { bl.x + so, bl.y + so },
                { br.x + so, br.y + so },
            };
            float const sa = (((Config::PanelAmmoShadowColor >> 24) & 0xFF) / 255.0f) * alpha;
            DrawTexturedQuad(device, shadowQuad, uv,
                D3DCOLOR_COLORVALUE(((Config::PanelAmmoShadowColor >> 16) & 0xFF) / 255.0f,
                    ((Config::PanelAmmoShadowColor >> 8) & 0xFF) / 255.0f,
                    (Config::PanelAmmoShadowColor & 0xFF) / 255.0f, sa));
        }

        float const ca = (((color >> 24) & 0xFF) / 255.0f) * alpha;
        DrawTexturedQuad(device, quad, uv,
            D3DCOLOR_COLORVALUE(((color >> 16) & 0xFF) / 255.0f,
                ((color >> 8) & 0xFF) / 255.0f,
                (color & 0xFF) / 255.0f, ca));
    };

    drawNumber(clipZone, static_cast<int>(currentWeapon.m_nAmmoInClip), Config::PanelAmmoClipSize,
        Config::PanelAmmoClipOffsetX, Config::PanelAmmoClipOffsetY, Config::PanelAmmoClipColor);
    drawNumber(reserveZone,
        static_cast<int>(currentWeapon.m_nAmmoTotal - currentWeapon.m_nAmmoInClip),
        Config::PanelAmmoReserveSize, Config::PanelAmmoReserveOffsetX,
        Config::PanelAmmoReserveOffsetY, Config::PanelAmmoReserveColor);
    if (prevSlot >= 0)
        drawNumber(prevZone, static_cast<int>(ped->m_aWeapons[prevSlot].m_nAmmoInClip),
            Config::PanelAmmoSideSize, Config::PanelAmmoPrevOffsetX, Config::PanelAmmoPrevOffsetY,
            Config::PanelAmmoSideColor);
    if (nextSlot >= 0)
        drawNumber(nextZone, static_cast<int>(ped->m_aWeapons[nextSlot].m_nAmmoInClip),
            Config::PanelAmmoSideSize, Config::PanelAmmoNextOffsetX, Config::PanelAmmoNextOffsetY,
            Config::PanelAmmoSideColor);

    DrawInfoText(device, m_text, projectPoint, panels, alpha);
}

void Panel3D::OnDeviceLost()
{
    m_blur.OnDeviceLost();
    m_text.OnDeviceLost();
    if (m_lineShader) {
        m_lineShader->Release();
        m_lineShader = nullptr;
    }
    if (m_barShader) {
        m_barShader->Release();
        m_barShader = nullptr;
    }
}
}
