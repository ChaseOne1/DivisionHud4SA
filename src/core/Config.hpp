#pragma once

#include <d3d9.h>

namespace division::core {

// Runtime tuning knobs, loaded from DivisionHud4SA/config/DivisionHud4SA.ini.
// The single source of truth for key names, defaults and INI sections lives in
// Config.cpp's descriptor table; this struct is just the store. Member names match
// the pre-table identifiers so call sites read naturally as `core::cfg.field`.
// Defaults below mirror the shipped INI values (tuned in-game); keep them in sync
// with DivisionHud4SA/config/DivisionHud4SA.ini.
struct Config {
    // [anchor]  Card position relative to the player's feet/orientation.
    float AnchorOffsetRight = -1.00f;
    float AnchorOffsetUp = 0.15f;
    float AnchorOffsetForward = 0.15f;

    // [follow]  How the card tracks the camera/player (smoothing, spring, bob, tilt, fade).
    float BoneInfluence = 0.3f;
    float SpringOmega = 182.0f;
    float AnchorSmooth = 400.0f;
    float SideSmooth = 400.0f;
    float YawSmooth = 450.0f;
    float PitchSmooth = 450.0f;
    float PitchDeadzone = 0.8f;
    float BaseYawOffset = -0.40f;
    float BasePitch = -0.03f;
    float BobAmplitude = 0.0f;
    float BobOmega = 110.0f;
    float TurnTiltGain = 0.00056f;
    float TurnTiltMax = 0.10f;
    float FadeInSpeed = 350.0f;
    float FadeOutSpeed = 500.0f;
    float FadeNearDist = 1.20f;
    float FadeNearFull = 1.80f;

    // [panel]  Card proportions and outline.
    float PanelLength = 0.80f;
    float PanelHeight = 0.28f;
    float PanelColumnLength = 0.48f;
    float PanelSkillHeight = 0.49f;
    float PanelGap = 0.01f;
    float PanelLineWidth = 1.0f;
    float PanelLineAlpha = 0.8f;
    float PanelLineAlphaBottom = 0.2f;

    // [ads]  Aim-down-sights: the card is blended onto a fixed screen-space quad.
    float PanelAdsDuration = 0.11f;
    float PanelAdsWidth = 0.17f;
    float PanelAdsCenterX = 0.68f;
    float PanelAdsCenterY = 0.70f;
    float PanelAdsYaw = 0.00f;
    float PanelAdsPitch = 0.09f;

    // [ammo]  Magazine / reserve / prev / next numbers on the left card.
    float PanelAmmoSplit = 0.65f;
    float PanelAmmoClipSplit = 0.60f;
    float PanelAmmoClipSize = 1.1f;
    float PanelAmmoReserveSize = 1.1f;
    float PanelAmmoSideSize = 0.80f;
    float PanelAmmoClipOffsetX = 0.0f;
    float PanelAmmoClipOffsetY = 0.0f;
    float PanelAmmoReserveOffsetX = 0.0f;
    float PanelAmmoReserveOffsetY = 0.05f;
    float PanelAmmoPrevOffsetX = 0.0f;
    float PanelAmmoPrevOffsetY = 0.0f;
    float PanelAmmoNextOffsetX = 0.0f;
    float PanelAmmoNextOffsetY = 0.0f;
    D3DCOLOR PanelAmmoClipColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR PanelAmmoReserveColor = D3DCOLOR_ARGB(119, 255, 255, 255);
    D3DCOLOR PanelAmmoSideColor = D3DCOLOR_ARGB(204, 255, 255, 255);
    D3DCOLOR PanelAmmoShadowColor = D3DCOLOR_ARGB(68, 0, 0, 0);
    float PanelAmmoShadowOffset = 2.5f;
    // Ammo panel animation.
    float PanelAmmoClipPop = 0.03f;
    float PanelAmmoFirePop = 0.05f;
    float PanelAmmoFirePopDecay = 8.0f;
    float PanelAmmoReloadTransition = 0.15f;
    D3DCOLOR PanelAmmoSideReloadColor = D3DCOLOR_ARGB(85, 255, 255, 255);

    // [weapon_preview]  Weapon-switch preview row (screen-space, top-centre).
    float WeaponPreviewCenterX = 0.5f;
    float WeaponPreviewCenterY = 0.16f;
    int WeaponPreviewMaxCount = 3;            // max panels shown (1..5)
    float WeaponPreviewIconHeight = 0.045f;
    float WeaponPreviewPanelAspect = 2.4f;
    float WeaponPreviewIconPad = 0.18f;
    float WeaponPreviewAmmoRatio = 0.55f;
    float WeaponPreviewGap = 0.006f;
    float WeaponPreviewMaxWidth = 0.72f;
    float WeaponPreviewActiveScale = 1.12f;
    D3DCOLOR WeaponPreviewActiveColor = D3DCOLOR_ARGB(255, 232, 135, 26);
    D3DCOLOR WeaponPreviewLineColor = D3DCOLOR_ARGB(204, 255, 255, 255);
    float WeaponPreviewLineWidth = 1.0f;
    D3DCOLOR WeaponPreviewTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    float WeaponPreviewTextSize = 0.85f;
    float WeaponPreviewHold = 0.9f;
    float WeaponPreviewFadeIn = 0.15f;
    float WeaponPreviewFadeOut = 0.25f;
    float WeaponPreviewSlideSpeed = 4.0f;

    // [vitals]  Health / armour / stamina / breath bars above the card.
    float PanelVitalsOffsetY = 0.08f;
    float PanelVitalsGap = 0.06f;
    float PanelVitalsLength = 1.0f;
    float PanelVitalsThicknessHealth = 0.06f;
    float PanelVitalsThicknessArmor = 0.10f;
    float PanelVitalsThicknessStamina = 0.05f;
    D3DCOLOR PanelVitalsHealthColor = D3DCOLOR_ARGB(255, 232, 135, 26);
    D3DCOLOR PanelVitalsHealthTrackColor = D3DCOLOR_ARGB(68, 0, 0, 0);
    D3DCOLOR PanelVitalsArmorColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR PanelVitalsStaminaColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR PanelVitalsBreathColor = D3DCOLOR_ARGB(255, 46, 155, 255);
    float PanelVitalsBreathHold = 1.0f;
    D3DCOLOR PanelVitalsCursorColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    float PanelVitalsCursorWidth = 0.006f;
    float PanelVitalsCursorScale = 1.4f;
    float PanelVitalsCursorDotGap = 0.02f;
    float PanelVitalsArmorUnit = 3.0f;
    float PanelVitalsArmorGap = 0.008f;
    float PanelVitalsStaminaFadeIn = 0.09f;
    float PanelVitalsStaminaFadeOut = 0.09f;
    float PanelVitalsStaminaHold = 1.0f;
    float PanelVitalsMoveFade = 0.18f;   // stack reflow / bar appear-disappear transition
    // Health damage indicator (red trailing bar) and change pulse.
    D3DCOLOR PanelVitalsHealthChipColor = D3DCOLOR_ARGB(255, 210, 45, 45);
    float PanelVitalsHealthChipHold = 1.0f;
    float PanelVitalsHealthChipSpeed = 0.5f;
    float PanelVitalsHealthPulseHold = 2.0f;
    float PanelVitalsHealthPulseScale = 1.1f;
    float PanelVitalsHealthPulseFadeIn = 0.05f;
    float PanelVitalsHealthPulseFadeOut = 0.25f;

    // [damage]  Damage-source direction indicators around the crosshair.
    float PanelDamageCenterX = 0.5f;
    float PanelDamageCenterY = 0.5f;
    float PanelDamageRadius = 90.0f;
    float PanelDamageArcThickness = 10.0f;
    float PanelDamageArcMaxDeg = 90.0f;
    float PanelDamageArcMinDeg = 23.0f;
    float PanelDamageArcGainDeg = 10.0f;
    float PanelDamageArcHold = 1.0f;
    float PanelDamageArcShrinkDeg = 45.0f;
    float PanelDamageForget = 3.0f;
    float PanelDamagePointerSize = 33.0f;
    float PanelDamagePointerWidth = 0.6f;
    D3DCOLOR PanelDamageColorArmor = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR PanelDamageColorHealth = D3DCOLOR_ARGB(255, 226, 45, 45);
    int PanelDamageMaxSources = 8;

    // [hud]  Info panel text/icons (money, clock, wanted count).
    float PanelHudTextSize = 0.6f;
    float PanelHudIconSize = 0.5f;
    D3DCOLOR PanelHudTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR PanelHudIconColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR PanelHudTextShadowColor = D3DCOLOR_ARGB(68, 0, 0, 0);
    float PanelHudTextShadowOffset = 2.5f;
    int PanelMoneyDigits = 10;
    float PanelSkillIconX = 0.05f;
    float PanelSkillValueX = 0.17f;
    float PanelClockIconX = 0.1f;
    float PanelClockValueX = 0.22f;
    float PanelWantedIconX = 0.68f;
    float PanelWantedValueX = 0.82f;

    // [wanted]  Wanted-tier progress overlay on the resource card.
    float PanelWantedBarX0 = 0.62f;
    float PanelWantedBarX1 = 1.0f;
    D3DCOLOR PanelWantedBarColor = D3DCOLOR_ARGB(0x33, 255, 255, 255); // fill mask
    float PanelWantedBarSpeed = 10.0f;
    float PanelWantedBreath = 0.7f;
    D3DCOLOR PanelWantedWaveColor = D3DCOLOR_ARGB(0x55, 255, 255, 255);
    float PanelWantedWaveWidth = 0.1f;
    float PanelWantedWaveSweep = 0.6f;
    float PanelWantedWaveFade = 0.3f;
    float PanelWantedWaveInterval = 1.0f;
    // "Star gained" flash: the wanted area turns this colour, then fades over the duration.
    D3DCOLOR PanelWantedFullColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    float PanelWantedFullDuration = 0.25f;

    // [blur]  Shared full-screen blur behind the cards.
    float BlurSpacing = 2.0f;
    int BlurIterations = 2;
    float BlurSaturation = 1.0f;
};

// The one config instance; read-only after load unless the INI changed on disk.
inline Config cfg;

void Load();
void Update();
}