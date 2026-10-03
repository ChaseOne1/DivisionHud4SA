#pragma once

#include <d3d9.h>

#include "render/TextRenderer.hpp"

class CPed;

namespace division::hud {

// Weapon-switch preview row: after the selected weapon slot changes, a sliding
// window of the player's occupied slots is shown across the top of the screen,
// the current weapon centre-highlighted. Each panel shows the weapon icon and its
// clip / reserve ammo. Appears and disappears with a fade. Screen-space, on foot.
class WeaponPreview
{
public:
    void Update();
    // `blurred` is the shared full-screen blur produced once per frame.
    void Draw(IDirect3DTexture9* blurred);
    void OnDeviceLost();

private:
    bool m_has_prev_slot = false;
    int m_prev_slot = -1;
    float m_hold = 0.0f;
    float m_alpha = 0.0f;

    float m_scroll = 0.0f;      // continuous reel position shown at screen centre
    float m_target = 0.0f;      // where the reel is heading
    float m_tween_from = 0.0f;  // reel position when the current switch started
    float m_tween = 1.0f;       // 0..1 progress of the current switch
    float m_tween_dur = 0.25f;  // seconds (set per switch)
    float m_bias = 1.0f;        // 2-weapon window bias (+1 = next on the right)
    float m_bias_from = 1.0f;   // bias when the current switch started
    bool m_scroll_init = false;
    int m_prev_current = -1;

    render::TextRenderer m_text;
    bool m_text_initialised = false;
    IDirect3DPixelShader9* m_line_shader = nullptr;
};

}
