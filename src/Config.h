#pragma once

#include <d3d9.h>

namespace Config {
inline float AnchorOffsetRight = -0.72f;
inline float AnchorOffsetUp = 0.35f;
inline float AnchorOffsetForward = 0.0f;

inline float BoneInfluence = 0.0f;

inline float PanelLength = 0.82f;
inline float PanelHeight = 0.50f;

inline float PanelColumnLength = 0.49f;
inline float PanelSkillHeight = 0.47f;
inline float PanelGap = 0.02f;
inline float PanelAmmoSplit = 0.75f;
inline float PanelAmmoClipSplit = 0.60f;
inline float PanelAmmoClipSize = 0.78f;
inline float PanelAmmoReserveSize = 0.58f;
inline float PanelAmmoSideSize = 0.44f;
inline float PanelAmmoClipOffsetX = 0.0f;
inline float PanelAmmoClipOffsetY = 0.0f;
inline float PanelAmmoReserveOffsetX = 0.0f;
inline float PanelAmmoReserveOffsetY = 0.0f;
inline float PanelAmmoPrevOffsetX = 0.0f;
inline float PanelAmmoPrevOffsetY = 0.0f;
inline float PanelAmmoNextOffsetX = 0.0f;
inline float PanelAmmoNextOffsetY = 0.0f;
inline D3DCOLOR PanelAmmoClipColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelAmmoReserveColor = D3DCOLOR_ARGB(255, 110, 110, 110);
inline D3DCOLOR PanelAmmoSideColor = D3DCOLOR_ARGB(255, 156, 156, 156);
inline D3DCOLOR PanelAmmoShadowColor = D3DCOLOR_ARGB(192, 0, 0, 0);
inline float PanelAmmoShadowOffset = 2.0f;

inline float PanelVitalsOffsetY = 0.04f;
inline float PanelVitalsGap = 0.03f;
inline float PanelVitalsLength = 1.0f;
inline float PanelVitalsThicknessHealth = 0.06f;
inline float PanelVitalsThicknessArmor = 0.10f;
inline float PanelVitalsThicknessStamina = 0.05f;
inline D3DCOLOR PanelVitalsHealthColor = D3DCOLOR_ARGB(255, 232, 135, 26);
inline D3DCOLOR PanelVitalsHealthTrackColor = D3DCOLOR_ARGB(192, 24, 24, 24);
inline D3DCOLOR PanelVitalsArmorColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelVitalsStaminaColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelVitalsBreathColor = D3DCOLOR_ARGB(255, 46, 155, 255);
inline float PanelVitalsBreathHold = 0.5f;
inline D3DCOLOR PanelVitalsCursorColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline float PanelVitalsCursorWidth = 0.004f;
inline float PanelVitalsCursorScale = 1.6f;
inline float PanelVitalsArmorUnit = 1.0f;
inline float PanelVitalsArmorGap = 0.0015f;
inline float PanelVitalsStaminaFadeIn = 0.40f;
inline float PanelVitalsStaminaFadeOut = 1.00f;
inline float PanelVitalsStaminaHold = 2.0f;

inline float PanelHudTextSize = 0.36f;
inline float PanelHudIconSize = 0.42f;
inline D3DCOLOR PanelHudTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline D3DCOLOR PanelHudIconColor = D3DCOLOR_ARGB(255, 255, 255, 255);
inline int PanelMoneyDigits = 8;
inline float PanelSkillIconX = 0.12f;
inline float PanelSkillValueX = 0.34f;
inline float PanelClockIconX = 0.08f;
inline float PanelClockValueX = 0.20f;
inline float PanelWantedIconX = 0.68f;
inline float PanelWantedValueX = 0.84f;
inline float PanelLineWidth = 2.0f;
inline float PanelLineAlpha = 1.0f;
inline float PanelLineAlphaBottom = 1.0f;
inline float PanelAdsDuration = 0.006f;
inline float PanelAdsWidth = 0.26f;
inline float PanelAdsCenterX = 0.68f;
inline float PanelAdsCenterY = 0.64f;
inline float PanelAdsYaw = -0.20f;
inline float PanelAdsPitch = 0.18f;

inline float BaseYawOffset = -0.25f;
inline float BasePitch = -0.03f;
inline float PitchDeadzone = 0.85f;
inline float PitchSmooth = 450.0f;
inline float YawSmooth = 450.0f;
inline float SideSmooth = 400.0f;

inline float SpringOmega = 218.0f;

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

inline D3DCOLOR BackTint = D3DCOLOR_ARGB(205, 255, 255, 255);

void Load();

void Update();
}
