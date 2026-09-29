#pragma once

#include "CVector.h"
#include "Spring.h"
#include "BlurBackground.h"
#include "TextRenderer.h"

namespace division {
class Panel3D
{
public:
    void Update();
    void Draw();
    void OnDeviceLost();

private:
    CVector m_position { };
    CVector m_velocity { };
    bool m_initialised = false;
    CVector m_target { };
    bool m_targetInitialised = false;
    CVector m_boneNeutral { };
    bool m_boneNeutralSet = false;
    float m_adsBlend = 0.0f;
    bool m_aiming = false;
    float m_adsFrom[4][2] = { };

    float m_alpha = 0.0f;
    float m_roll = 0.0f;
    float m_bobPhase = 0.0f;
    float m_pitch = 0.0f;
    bool m_pitchInitialised = false;
    float m_yaw = 0.0f;
    bool m_yawInitialised = false;
    CVector m_side { };
    bool m_sideInitialised = false;
    CVector m_forward { };
    bool m_forwardInitialised = false;
    float m_prevHeading = 0.0f;
    bool m_hasPrevHeading = false;

    BlurBackground m_blur;
    TextRenderer m_text;
    bool m_textInitialised = false;
    float m_staminaAlpha = 0.0f;
    float m_staminaHold = 0.0f;
    float m_breathAlpha = 0.0f;
    float m_breathHold = 0.0f;
    IDirect3DPixelShader9* m_lineShader = nullptr;
    IDirect3DPixelShader9* m_barShader = nullptr;
};
}
