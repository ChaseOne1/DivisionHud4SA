#pragma once

#include <d3d9.h>

namespace division::core {
inline float AnchorOffsetRight = -1.00f;
inline float AnchorOffsetUp = 0.15f;
inline float AnchorOffsetForward = 0.15f;

inline float BoneInfluence = 0.3f;

inline float PanelLength = 0.80f;
inline float PanelHeight = 0.28f;

inline float PanelColumnLength = 0.48f;
inline float PanelSkillHeight = 0.49f;
inline float PanelGap = 0.01f;
inline float PanelAmmoSplit = 0.65f;
inline float PanelAmmoClipSplit = 0.60f;
inline float PanelAmmoClipSize = 1.1f;
inline float PanelAmmoReserveSize = 1.1f;
inline float PanelAmmoSideSize = 0.80f;
inline float PanelAmmoClipOffsetX = 0.0f;
inline float PanelAmmoClipOffsetY = 0.0f;
inline float PanelAmmoReserveOffsetX = 0.0f;
inline float PanelAmmoReserveOffsetY = 0.05f;
inline float PanelAmmoPrevOffsetX = 0.0f;
inline float PanelAmmoPrevOffsetY = 0.0f;
inline float PanelAmmoNextOffsetX = 0.0f;
inline float PanelAmmoNextOffsetY = 0.0f;
inline D3DCOLOR PanelAmmoClipColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelAmmoReserveColor = D3DCOLOR_ARGB(119, 255, 255, 255);
inline D3DCOLOR PanelAmmoSideColor = D3DCOLOR_ARGB(204, 255, 255, 255);
inline D3DCOLOR PanelAmmoShadowColor = D3DCOLOR_ARGB(68, 0, 0, 0);
inline float PanelAmmoShadowOffset = 2.0f;
// Ammo panel animation.
inline float PanelAmmoClipPop = 0.10f;
inline float PanelAmmoFirePop = 0.06f;
inline float PanelAmmoFirePopDecay = 6.0f;
inline float PanelAmmoReloadTransition = 0.25f;
inline D3DCOLOR PanelAmmoSideReloadColor = D3DCOLOR_ARGB(85, 255, 255, 255);

inline float PanelVitalsOffsetY = 0.04f;
inline float PanelVitalsGap = 0.05f;
inline float PanelVitalsLength = 1.0f;
inline float PanelVitalsThicknessHealth = 0.06f;
inline float PanelVitalsThicknessArmor = 0.15f;
inline float PanelVitalsThicknessStamina = 0.05f;
inline D3DCOLOR PanelVitalsHealthColor = D3DCOLOR_ARGB(255, 232, 135, 26);
inline D3DCOLOR PanelVitalsHealthTrackColor = D3DCOLOR_ARGB(68, 0, 0, 0);
inline D3DCOLOR PanelVitalsArmorColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelVitalsStaminaColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelVitalsBreathColor = D3DCOLOR_ARGB(255, 46, 155, 255);
inline float PanelVitalsBreathHold = 1.0f;
inline D3DCOLOR PanelVitalsCursorColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline float PanelVitalsCursorWidth = 0.004f;
inline float PanelVitalsCursorScale = 1.6f;
inline float PanelVitalsCursorDotGap = 0.012f;
inline float PanelVitalsArmorUnit = 3.0f;
inline float PanelVitalsArmorGap = 0.008f;
inline float PanelVitalsStaminaFadeIn = 0.09f;
inline float PanelVitalsStaminaFadeOut = 0.09f;
inline float PanelVitalsStaminaHold = 1.0f;
inline float PanelVitalsMoveFade = 0.18f;   // stack reflow / bar appear-disappear transition
// Health damage indicator (red trailing bar) and change pulse.
inline D3DCOLOR PanelVitalsHealthChipColor = D3DCOLOR_ARGB(255, 210, 45, 45);
inline float PanelVitalsHealthChipHold = 0.5f;
inline float PanelVitalsHealthChipSpeed = 0.8f;
inline float PanelVitalsHealthPulseHold = 0.15f;
inline float PanelVitalsHealthPulseScale = 0.6f;
inline float PanelVitalsHealthPulseFadeIn = 0.05f;
inline float PanelVitalsHealthPulseFadeOut = 0.25f;

inline float PanelHudTextSize = 0.6f;
inline float PanelHudIconSize = 0.5f;
inline D3DCOLOR PanelHudTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelHudIconColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline int PanelMoneyDigits = 10;
inline float PanelSkillIconX = 0.05f;
inline float PanelSkillValueX = 0.17f;
inline float PanelClockIconX = 0.1f;
inline float PanelClockValueX = 0.22f;
inline float PanelWantedIconX = 0.68f;
inline float PanelWantedValueX = 0.82f;
inline float PanelLineWidth = 1.0f;
inline float PanelLineAlpha = 0.8f;
inline float PanelLineAlphaBottom = 0.2f;
inline float PanelAdsDuration = 0.11f;
inline float PanelAdsWidth = 0.17f;
inline float PanelAdsCenterX = 0.68f;
inline float PanelAdsCenterY = 0.70f;
inline float PanelAdsYaw = -0.05f;
inline float PanelAdsPitch = 0.05f;

inline float BaseYawOffset = -0.40f;
inline float BasePitch = -0.03f;
inline float PitchDeadzone = 0.8f;
inline float PitchSmooth = 450.0f;
inline float YawSmooth = 450.0f;
inline float SideSmooth = 400.0f;

inline float SpringOmega = 182.0f;

inline float AnchorSmooth = 400.0f;

inline float BobAmplitude = 0.0f;
inline float BobOmega = 110.0f;

inline float TurnTiltGain = 0.00056f;
inline float TurnTiltMax = 0.10f;

inline float FadeInSpeed = 350.0f;
inline float FadeOutSpeed = 500.0f;

inline float FadeNearDist = 1.20f;
inline float FadeNearFull = 1.80f;

inline float BlurSpacing = 2.0f;
inline int BlurIterations = 2;
inline float BlurSaturation = 1.0f;

inline D3DCOLOR BackTint = D3DCOLOR_ARGB(255, 255, 255, 255);

void Load();

void Update();
}
