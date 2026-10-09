#pragma once

#include <d3d9.h>
#include <vector>

#include "CVector.h"
#include "core/Spring.hpp"
#include "render/Surface.hpp"
#include "render/TextRenderer.hpp"
#include "hud/DamageIndicators.hpp"
#include "hud/widgets/AmmoPanel.hpp"
#include "hud/widgets/Info.hpp"
#include "hud/widgets/Vitals.hpp"
#include "hud/widgets/WantedBar.hpp"

namespace division::hud {

// Anchors, orients and renders the whole HUD card.
class Panel3D
{
public:
    void Update();
    // `blurred` is the shared full-screen blur produced once per frame.
    void Draw(IDirect3DTexture9* blurred);
    void OnDeviceLost();

private:
    CVector m_position { };
    CVector m_velocity { };
    bool m_initialised = false;
    CVector m_target { };
    bool m_target_initialised = false;
    CVector m_bone_neutral { };
    bool m_bone_neutral_set = false;

    float m_ads_blend = 0.0f;
    bool m_aiming = false;
    render::ScreenPoint m_ads_from[4] { };

    float m_alpha = 0.0f;
    float m_roll = 0.0f;
    float m_bob_phase = 0.0f;
    float m_pitch = 0.0f;
    bool m_pitch_initialised = false;
    float m_yaw = 0.0f;
    bool m_yaw_initialised = false;
    CVector m_side { };
    bool m_side_initialised = false;
    CVector m_forward { };
    bool m_forward_initialised = false;
    float m_prev_heading = 0.0f;
    bool m_has_prev_heading = false;

    render::TextRenderer m_text;
    Vitals m_vitals;
    AmmoPanel m_ammo;
    WantedBar m_wanted;
    DamageIndicators m_damage;
    std::vector<render::BarVertex> m_bar_vertices;
    bool m_text_initialised = false;
    IDirect3DPixelShader9* m_line_shader = nullptr;
    IDirect3DPixelShader9* m_bar_shader = nullptr;
    IDirect3DPixelShader9* m_wave_shader = nullptr;
};

}
