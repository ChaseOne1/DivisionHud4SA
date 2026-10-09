#include "plugin.h"

#include "common.h"
#include "CPed.h"
#include "CCamera.h"
#include "CSprite.h"
#include "CWorld.h"
#include "ePedBones.h"
#include "eCamMode.h"

#include <algorithm>
#include <cmath>
#include <d3d9.h>

#include "hud/Panel3D.hpp"
#include "core/Config.hpp"
#include "core/Math.hpp"
#include "core/Animation.hpp"
#include "hud/widgets/Layout.hpp"
#include "hud/widgets/PanelFrame.hpp"
#include "render/Pipeline.hpp"
#include "render/Primitives.hpp"
#include "render/Shader.hpp"
#include "render/Surface.hpp"

namespace division::hud {
using namespace render;

namespace {
    using core::PI_F;
    using core::WrapAngle;

    CVector CameraRight() { return TheCamera.m_mCameraMatrix.GetRight(); }
    CVector CameraForward() { return TheCamera.m_mCameraMatrix.GetForward(); }

    bool ProjectToScreen(CVector const& world, ScreenPoint& out)
    {
        RwV3d projected { };
        float w = 0.0f, h = 0.0f;
        if (!CSprite::CalcScreenCoors(world, &projected, &w, &h, false, false)) return false;
        out.x = projected.x;
        out.y = projected.y;
        return true;
    }

    bool IsAimMode(eCamMode mode)
    {
        switch (mode) {
        case MODE_AIMWEAPON:
        case MODE_AIMWEAPON_FROMCAR:
        case MODE_AIMWEAPON_ATTACHED:
        case MODE_SNIPER:
        case MODE_ROCKETLAUNCHER:
        case MODE_1STPERSON:
        case MODE_M16_1STPERSON:
        case MODE_SNIPER_RUNABOUT:
        case MODE_ROCKETLAUNCHER_RUNABOUT:
        case MODE_1STPERSON_RUNABOUT:
        case MODE_M16_1STPERSON_RUNABOUT:
        case MODE_ROCKETLAUNCHER_HS:
        case MODE_ROCKETLAUNCHER_RUNABOUT_HS:
        case MODE_HELICANNON_1STPERSON:
            return true;
        default:
            return false;
        }
    }

    bool IsPlayerAiming(CPed* ped)
    {
        if (ped->bIsAimingGun) return true;
        if (ped->m_ePedState == PEDSTATE_AIMGUN) return true;

        // Do NOT key on PEDSTATE_SNIPER_MODE: the game reuses it as the generic
        // "player is aiming" state for sniper / rocket launcher / HS / camera weapons
        // (see CTaskSimplePlayerOnFoot::ProcessPlayerWeapon, which pushes 0Ch), and it
        // is left set after the aim key is released. The camera mode below is set and
        // cleared with the aim key, so it is the reliable exit signal.
        unsigned char const active_cam = TheCamera.m_nActiveCam;
        return IsAimMode(TheCamera.m_aCams[active_cam < 3 ? active_cam : 0].m_nMode);
    }

    void ProjectRect(PanelSurface const& surface, Rect const& rect, ScreenPoint (&out)[4])
    {
        out[0] = surface.Map(rect.x0, rect.y1);
        out[1] = surface.Map(rect.x1, rect.y1);
        out[2] = surface.Map(rect.x0, rect.y0);
        out[3] = surface.Map(rect.x1, rect.y0);
    }
}

void Panel3D::Update()
{
    core::Update();

    CPed* ped = FindPlayerPed(0);
    bool const in_vehicle = FindPlayerVehicle(0, false) != nullptr;

    float dt = core::FrameDeltaSeconds();

    bool const aiming = ped && !in_vehicle && IsPlayerAiming(ped);
    m_aiming = aiming;

    float const ads_target = aiming ? 1.0f : 0.0f;
    float const ads_step = dt / std::max(0.01f, core::cfg.PanelAdsDuration);
    m_ads_blend = (m_ads_blend < ads_target) ? std::min(ads_target, m_ads_blend + ads_step)
                                             : std::max(ads_target, m_ads_blend - ads_step);

    if (ped) {
        CVector raw_side = CameraRight();
        raw_side.z = 0.0f;
        if (raw_side.MagnitudeSqr() < 1e-6f)
            raw_side = CVector(1.0f, 0.0f, 0.0f);
        raw_side.Normalize();

        if (!m_side_initialised) {
            m_side = raw_side;
            m_side_initialised = true;
        } else {
            m_side += (raw_side - m_side) * std::min(1.0f, dt * core::cfg.SideSmooth);
            m_side.Normalize();
        }

        CVector raw_forward = CameraForward();
        raw_forward.z = 0.0f;
        if (raw_forward.MagnitudeSqr() < 1e-6f)
            raw_forward = CVector(0.0f, 1.0f, 0.0f);
        raw_forward.Normalize();

        if (!m_forward_initialised) {
            m_forward = raw_forward;
            m_forward_initialised = true;
        } else {
            m_forward += (raw_forward - m_forward) * std::min(1.0f, dt * core::cfg.SideSmooth);
            m_forward.Normalize();
        }

        CVector base = ped->GetPosition();
        if (core::cfg.BoneInfluence > 0.0f && ped->m_pRwClump) {
            RwV3d bone { };
            ped->GetBonePosition(bone, BONE_UPPERTORSO, false);
            CVector const bone_offset = CVector(bone) - base;
            if (!m_bone_neutral_set) {
                m_bone_neutral = bone_offset;
                m_bone_neutral_set = true;
            }
            base += (bone_offset - m_bone_neutral) * core::cfg.BoneInfluence;
        }

        CVector target = base;
        target += m_side * core::cfg.AnchorOffsetRight;
        target += m_forward * core::cfg.AnchorOffsetForward;
        target += CVector(0.0f, 0.0f, 1.0f) * core::cfg.AnchorOffsetUp;

        if (!m_target_initialised) {
            m_target = target;
            m_target_initialised = true;
        } else {
            m_target += (target - m_target) * std::min(1.0f, dt * core::cfg.AnchorSmooth);
        }

        if (!m_initialised) {
            m_position = m_target;
            m_velocity = CVector(0.0f);
            m_initialised = true;
        } else {
            core::Damp(m_position, m_velocity, m_target, core::cfg.SpringOmega, dt);
        }

        float const heading = ped->GetHeading();
        float turn = 0.0f;
        if (m_has_prev_heading)
            turn = std::clamp(WrapAngle(heading - m_prev_heading) / dt, -400.0f, 400.0f);
        m_prev_heading = heading;
        m_has_prev_heading = true;

        float const target_roll = std::clamp(-turn * core::cfg.TurnTiltGain, -core::cfg.TurnTiltMax, core::cfg.TurnTiltMax);
        m_roll += (target_roll - m_roll) * std::min(1.0f, dt * 450.0f);

        CVector forward = CameraForward();
        forward.Normalize();

        float const raw_yaw = std::atan2(forward.y, forward.x);
        if (!m_yaw_initialised) {
            m_yaw = raw_yaw;
            m_yaw_initialised = true;
        } else {
            m_yaw += WrapAngle(raw_yaw - m_yaw) * std::min(1.0f, dt * core::cfg.YawSmooth);
        }

        float const camera_pitch = std::asin(std::clamp(forward.z, -1.0f, 1.0f));
        float const excess = std::max(0.0f, std::fabs(camera_pitch) - core::cfg.PitchDeadzone);
        float const target_pitch = core::cfg.BasePitch - std::copysign(excess, camera_pitch);

        if (!m_pitch_initialised) {
            m_pitch = target_pitch;
            m_pitch_initialised = true;
        } else {
            m_pitch += (target_pitch - m_pitch) * std::min(1.0f, dt * core::cfg.PitchSmooth);
        }

        m_bob_phase += dt * core::cfg.BobOmega;
    }

    float const target_alpha = (ped && !in_vehicle) ? 1.0f : 0.0f;
    float const rate = (target_alpha > m_alpha) ? core::cfg.FadeInSpeed : core::cfg.FadeOutSpeed;
    m_alpha += (target_alpha - m_alpha) * std::min(1.0f, dt * rate);

    m_vitals.Update(ped, dt);
    m_ammo.Update(ped, dt);
    m_wanted.Update(CWorld::Players[CWorld::PlayerInFocus].m_PlayerData.m_pWanted, dt);
    m_damage.Update(ped, dt);
}

void Panel3D::Draw(IDirect3DTexture9* blurred)
{
    if (m_alpha < 0.01f || !m_initialised) return;

    CPed* ped = FindPlayerPed(0);
    if (!ped || FindPlayerVehicle(0, false)) return;

    IDirect3DDevice9* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    if (!device) return;

    float alpha = m_alpha;
    {
        CVector const& camera_position = TheCamera.GetPosition();
        float const distance = CVector::Distance(m_position, camera_position);
        float near_fade = 1.0f;
        if (distance < core::cfg.FadeNearDist)
            near_fade = 0.0f;
        else if (distance < core::cfg.FadeNearFull)
            near_fade = (distance - core::cfg.FadeNearDist) / (core::cfg.FadeNearFull - core::cfg.FadeNearDist);
        near_fade += (1.0f - near_fade) * m_ads_blend;
        if (near_fade < 0.01f) return;
        alpha *= near_fade;
    }

    int const screen_width = RsGlobal.maximumWidth;
    int const screen_height = RsGlobal.maximumHeight;
    if (screen_width <= 0 || screen_height <= 0) return;

    float const yaw = m_yaw + PI_F + core::cfg.BaseYawOffset;
    float const pitch = m_pitch;

    CVector normal(std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch));
    normal.Normalize();

    CVector const world_up(0.0f, 0.0f, 1.0f);
    CVector right = world_up.Cross(normal);
    if (right.MagnitudeSqr() < 1e-6f)
        right = CVector(1.0f, 0.0f, 0.0f);
    right.Normalize();
    CVector up = normal.Cross(right);
    up.Normalize();

    {
        CVector const rolled_right = right * std::cos(m_roll) + up * std::sin(m_roll);
        CVector const rolled_up = up * std::cos(m_roll) - right * std::sin(m_roll);
        right = rolled_right;
        up = rolled_up;
    }

    CVector center = m_position;
    center.z += std::sin(m_bob_phase) * core::cfg.BobAmplitude;

    PanelRects const panels = Compute(core::cfg.PanelColumnLength, core::cfg.PanelSkillHeight, core::cfg.PanelGap);
    AmmoZones const ammo_zones = ComputeAmmoZones(panels.ammo, core::cfg.PanelAmmoSplit, core::cfg.PanelAmmoClipSplit);

    float const screen_w = static_cast<float>(screen_width);
    float const screen_h = static_cast<float>(screen_height);

    auto to_world = [&](float nx, float ny) {
        CVector point = center;
        point += right * ((nx - 0.5f) * core::cfg.PanelLength);
        point += up * ((ny - 0.5f) * core::cfg.PanelHeight);
        return point;
    };

    float const ads_width = screen_w * core::cfg.PanelAdsWidth;
    float const ads_height = ads_width * (core::cfg.PanelHeight / std::max(core::cfg.PanelLength, 0.0001f));
    float const ads_center_x = screen_w * core::cfg.PanelAdsCenterX;
    float const ads_center_y = screen_h * core::cfg.PanelAdsCenterY;

    float const corner_local[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    ScreenPoint world_quad[4];
    for (int i = 0; i < 4; ++i) {
        if (!ProjectToScreen(to_world(corner_local[i][0], corner_local[i][1]), world_quad[i])) {
            world_quad[i].x = ads_center_x;
            world_quad[i].y = ads_center_y;
        }
    }

    float const depth = ads_width * 2.0f;
    float const cp = std::cos(core::cfg.PanelAdsPitch);
    float const sp = std::sin(core::cfg.PanelAdsPitch);
    float const cyaw = std::cos(core::cfg.PanelAdsYaw);
    float const syaw = std::sin(core::cfg.PanelAdsYaw);
    ScreenPoint ads_quad[4];
    for (int i = 0; i < 4; ++i) {
        float const x = (corner_local[i][0] - 0.5f) * ads_width;
        float const y = (corner_local[i][1] - 0.5f) * ads_height;

        float const yaw_x = x * cyaw;
        float const yaw_z = -x * syaw;

        float const pitch_y = y * cp + yaw_z * sp;
        float const pitch_z = -y * sp + yaw_z * cp;

        float const perspective = depth / (depth - pitch_z);
        ads_quad[i].x = ads_center_x + yaw_x * perspective;
        ads_quad[i].y = ads_center_y - pitch_y * perspective;
    }

    if (!m_aiming) {
        for (int i = 0; i < 4; ++i)
            m_ads_from[i] = world_quad[i];
    }

    ScreenPoint quad[4];
    for (int i = 0; i < 4; ++i) {
        quad[i].x = m_ads_from[i].x + (ads_quad[i].x - m_ads_from[i].x) * m_ads_blend;
        quad[i].y = m_ads_from[i].y + (ads_quad[i].y - m_ads_from[i].y) * m_ads_blend;
    }

    if (!blurred) return;

    PanelSurface surface;
    surface.SetCorners(quad[3], quad[2], quad[1], quad[0]);

    DeviceStateGuard guard(device);

    ApplyScreenPipeline(device);

    ScreenPoint ammo_quad[4], skill_quad[4], resource_quad[4];
    ProjectRect(surface, panels.ammo, ammo_quad);
    ProjectRect(surface, panels.skill, skill_quad);
    ProjectRect(surface, panels.resource, resource_quad);

    auto draw_backdrop = [&](ScreenPoint const(&points)[4]) {
        float uv[4][2];
        for (int i = 0; i < 4; ++i) {
            uv[i][0] = points[i].x / screen_w;
            uv[i][1] = points[i].y / screen_h;
        }
        DrawBackdrop(device, points, uv, alpha);
    };

    device->SetTexture(0, blurred);
    draw_backdrop(ammo_quad);
    draw_backdrop(skill_quad);
    draw_backdrop(resource_quad);

    device->SetTexture(0, nullptr);
    D3DCOLOR const line_color = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, alpha * core::cfg.PanelLineAlpha);
    D3DCOLOR const bottom_color = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, alpha * core::cfg.PanelLineAlphaBottom);
    float const line_thickness = core::cfg.PanelLineWidth;

    if (!m_line_shader)
        m_line_shader = CreateLineShader(device);
    device->SetPixelShader(m_line_shader);

    ScreenPoint const ammo_center = surface.Map((panels.ammo.x0 + panels.ammo.x1) * 0.5f, (panels.ammo.y0 + panels.ammo.y1) * 0.5f);
    ScreenPoint const skill_center = surface.Map((panels.skill.x0 + panels.skill.x1) * 0.5f, (panels.skill.y0 + panels.skill.y1) * 0.5f);
    ScreenPoint const resource_center = surface.Map((panels.resource.x0 + panels.resource.x1) * 0.5f, (panels.resource.y0 + panels.resource.y1) * 0.5f);

    auto draw_edge = [&](Rect const& rect, bool top, ScreenPoint const& interior, D3DCOLOR color) {
        float const y = top ? rect.y1 : rect.y0;
        DrawEdgeLine(device, surface.Map(rect.x0, y), surface.Map(rect.x1, y), interior, line_thickness, color);
    };

    draw_edge(panels.ammo, true, ammo_center, line_color);
    draw_edge(panels.ammo, false, ammo_center, line_color);
    draw_edge(panels.skill, true, skill_center, line_color);
    draw_edge(panels.skill, false, skill_center, bottom_color);
    draw_edge(panels.resource, true, resource_center, line_color);
    draw_edge(panels.resource, false, resource_center, bottom_color);

    DrawLine(device, surface.Map(ammo_zones.divider_x, panels.ammo.y1), surface.Map(ammo_zones.divider_x, panels.ammo.y0),
        line_thickness, line_color);

    if (!m_bar_shader)
        m_bar_shader = CreateBarShader(device);
    device->SetPixelShader(m_bar_shader);
    device->SetFVF(BAR_FVF);
    m_vitals.Draw(device, surface, alpha, m_bar_vertices);
    m_wanted.DrawMask(device, surface, panels, alpha, m_bar_vertices);

    if (!m_wave_shader)
        m_wave_shader = CreateWaveShader(device);
    if (m_wave_shader) {
        device->SetPixelShader(m_wave_shader);
        device->SetFVF(VERTEX_FVF);
        m_wanted.DrawWave(device, surface, panels, alpha);
        device->SetPixelShader(m_bar_shader);
        device->SetFVF(BAR_FVF);
    }

    DrawInfoIcons(device, surface, panels, alpha, m_bar_vertices);
    m_damage.Draw(device, m_bar_vertices, alpha);
    device->SetFVF(VERTEX_FVF);

    if (!m_text_initialised) {
        m_text.Initialize(PLUGIN_PATH("DivisionHud4SA/fonts/Borda Medium.ttf"), 128);
        m_text_initialised = true;
    }

    device->SetPixelShader(nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    PanelFrame frame;
    frame.surface = surface;
    m_ammo.Draw(device, m_text, frame, ammo_zones, ped, alpha);
    DrawInfoText(device, m_text, surface, panels, alpha);
}

void Panel3D::OnDeviceLost()
{
    m_text.OnDeviceLost();
    if (m_line_shader) {
        m_line_shader->Release();
        m_line_shader = nullptr;
    }
    if (m_bar_shader) {
        m_bar_shader->Release();
        m_bar_shader = nullptr;
    }
    if (m_wave_shader) {
        m_wave_shader->Release();
        m_wave_shader = nullptr;
    }
}
}
