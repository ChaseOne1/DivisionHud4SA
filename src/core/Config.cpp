#include "core/Config.hpp"
#include "plugin.h"

#include <cstdlib>
#include <fstream>
#include <string>

namespace division::core {
namespace {
    char const SECTION[] = "DivisionHud4SA";

    std::string g_path;
    ULONGLONG g_lastWrite = 0;
    unsigned int g_frameCounter = 0;

    std::string const& IniPath()
    {
        if (g_path.empty())
            g_path = PLUGIN_PATH("DivisionHud4SA.ini");
        return g_path;
    }

    float ReadFloat(char const* key, float default_value)
    {
        char buffer[64] { };
        GetPrivateProfileStringA(SECTION, key, "", buffer, sizeof(buffer), IniPath().c_str());
        if (buffer[0] == '\0')
            return default_value;
        return static_cast<float>(std::atof(buffer));
    }

    int ReadInt(char const* key, int default_value)
    {
        return static_cast<int>(GetPrivateProfileIntA(SECTION, key, default_value, IniPath().c_str()));
    }

    D3DCOLOR ReadHexColor(char const* key, D3DCOLOR default_value)
    {
        char buffer[64] { };
        GetPrivateProfileStringA(SECTION, key, "", buffer, sizeof(buffer), IniPath().c_str());
        if (buffer[0] == '\0')
            return default_value;
        return static_cast<D3DCOLOR>(std::strtoul(buffer, nullptr, 16));
    }

    void WriteDefaults()
    {
        std::ofstream file(IniPath().c_str());
        if (!file)
            return;

        file << "[DivisionHud4SA]\n"
                "anchor_offset_right = -0.72\n"
                "anchor_offset_up = 0.35\n"
                "anchor_offset_forward = 0.0\n"
                "bone_influence = 0.0\n"
                "panel_length = 0.82\n"
                "panel_height = 0.50\n"
                "panel_column_length = 0.49\n"
                "panel_skill_height = 0.47\n"
                "panel_gap = 0.02\n"
                "panel_ammo_split = 0.75\n"
                "panel_ammo_clip_split = 0.60\n"
                "panel_ammo_clip_size = 0.78\n"
                "panel_ammo_reserve_size = 0.58\n"
                "panel_ammo_side_size = 0.44\n"
                "panel_ammo_clip_x = 0.0\n"
                "panel_ammo_clip_y = 0.0\n"
                "panel_ammo_reserve_x = 0.0\n"
                "panel_ammo_reserve_y = 0.0\n"
                "panel_ammo_prev_x = 0.0\n"
                "panel_ammo_prev_y = 0.0\n"
                "panel_ammo_next_x = 0.0\n"
                "panel_ammo_next_y = 0.0\n"
                "panel_ammo_clip_color = FFFFFFFF\n"
                "panel_ammo_reserve_color = FF6E6E6E\n"
                "panel_ammo_side_color = FF9C9C9C\n"
                "panel_ammo_shadow_color = C0000000\n"
                "panel_ammo_shadow_offset = 2.0\n"
                "panel_vitals_offset_y = 0.04\n"
                "panel_vitals_gap = 0.03\n"
                "panel_vitals_length = 1.0\n"
                "panel_vitals_thickness_health = 0.06\n"
                "panel_vitals_thickness_armor = 0.10\n"
                "panel_vitals_thickness_stamina = 0.05\n"
                "panel_vitals_health_color = FFE8871A\n"
                "panel_vitals_health_track_color = C0181818\n"
                "panel_vitals_armor_color = FFFFFFFF\n"
                "panel_vitals_stamina_color = FFFFFFFF\n"
                "panel_vitals_breath_color = FF2E9BFF\n"
                "panel_vitals_breath_hold = 0.5\n"
                "panel_vitals_cursor_color = FFFFFFFF\n"
                "panel_vitals_cursor_width = 0.004\n"
                "panel_vitals_cursor_scale = 1.6\n"
                "panel_vitals_armor_unit = 1.0\n"
                "panel_vitals_armor_gap = 0.0015\n"
                "panel_vitals_stamina_fade_in = 0.40\n"
                "panel_vitals_stamina_fade_out = 1.00\n"
                "panel_vitals_stamina_hold = 2.0\n"
                "panel_hud_text_size = 0.36\n"
                "panel_hud_icon_size = 0.42\n"
                "panel_hud_text_color = FFFFFFFF\n"
                "panel_hud_icon_color = FFFFFFFF\n"
                "panel_money_digits = 8\n"
                "panel_skill_icon_x = 0.12\n"
                "panel_skill_value_x = 0.34\n"
                "panel_clock_icon_x = 0.08\n"
                "panel_clock_value_x = 0.20\n"
                "panel_wanted_icon_x = 0.68\n"
                "panel_wanted_value_x = 0.84\n"
                "panel_line_width = 2.0\n"
                "panel_line_alpha = 1.0\n"
                "panel_line_alpha_bottom = 1.0\n"
                "panel_ads_duration = 0.006\n"
                "panel_ads_width = 0.26\n"
                "panel_ads_center_x = 0.68\n"
                "panel_ads_center_y = 0.64\n"
                "panel_ads_yaw = -0.20\n"
                "panel_ads_pitch = 0.18\n"
                "base_yaw_offset = -0.25\n"
                "base_pitch = -0.03\n"
                "pitch_deadzone = 0.85\n"
                "pitch_smooth = 450.0\n"
                "yaw_smooth = 450.0\n"
                "side_smooth = 400.0\n"
                "spring_omega = 218.0\n"
                "anchor_smooth = 400.0\n"
                "bob_amplitude = 0.0\n"
                "bob_omega = 110.0\n"
                "turn_tilt_gain = 0.00056\n"
                "turn_tilt_max = 0.10\n"
                "fade_in_speed = 350.0\n"
                "fade_out_speed = 500.0\n"
                "fade_near_dist = 1.20\n"
                "fade_near_full = 1.80\n"
                "blur_spacing = 2.0\n"
                "blur_iterations = 2\n"
                "blur_saturation = 1.0\n"
                "back_tint = CDFFFFFF\n";
    }
}

void Load()
{
    {
        std::ifstream existing(IniPath().c_str());
        if (!existing.good())
            WriteDefaults();
    }

    AnchorOffsetRight = ReadFloat("anchor_offset_right", AnchorOffsetRight);
    AnchorOffsetUp = ReadFloat("anchor_offset_up", AnchorOffsetUp);
    AnchorOffsetForward = ReadFloat("anchor_offset_forward", AnchorOffsetForward);
    BoneInfluence = ReadFloat("bone_influence", BoneInfluence);
    PanelLength = ReadFloat("panel_length", PanelLength);
    PanelHeight = ReadFloat("panel_height", PanelHeight);
    PanelColumnLength = ReadFloat("panel_column_length", PanelColumnLength);
    PanelSkillHeight = ReadFloat("panel_skill_height", PanelSkillHeight);
    PanelGap = ReadFloat("panel_gap", PanelGap);
    PanelAmmoSplit = ReadFloat("panel_ammo_split", PanelAmmoSplit);
    PanelAmmoClipSplit = ReadFloat("panel_ammo_clip_split", PanelAmmoClipSplit);
    PanelAmmoClipSize = ReadFloat("panel_ammo_clip_size", PanelAmmoClipSize);
    PanelAmmoReserveSize = ReadFloat("panel_ammo_reserve_size", PanelAmmoReserveSize);
    PanelAmmoSideSize = ReadFloat("panel_ammo_side_size", PanelAmmoSideSize);
    PanelAmmoClipOffsetX = ReadFloat("panel_ammo_clip_x", PanelAmmoClipOffsetX);
    PanelAmmoClipOffsetY = ReadFloat("panel_ammo_clip_y", PanelAmmoClipOffsetY);
    PanelAmmoReserveOffsetX = ReadFloat("panel_ammo_reserve_x", PanelAmmoReserveOffsetX);
    PanelAmmoReserveOffsetY = ReadFloat("panel_ammo_reserve_y", PanelAmmoReserveOffsetY);
    PanelAmmoPrevOffsetX = ReadFloat("panel_ammo_prev_x", PanelAmmoPrevOffsetX);
    PanelAmmoPrevOffsetY = ReadFloat("panel_ammo_prev_y", PanelAmmoPrevOffsetY);
    PanelAmmoNextOffsetX = ReadFloat("panel_ammo_next_x", PanelAmmoNextOffsetX);
    PanelAmmoNextOffsetY = ReadFloat("panel_ammo_next_y", PanelAmmoNextOffsetY);
    PanelAmmoClipColor = ReadHexColor("panel_ammo_clip_color", PanelAmmoClipColor);
    PanelAmmoReserveColor = ReadHexColor("panel_ammo_reserve_color", PanelAmmoReserveColor);
    PanelAmmoSideColor = ReadHexColor("panel_ammo_side_color", PanelAmmoSideColor);
    PanelAmmoShadowColor = ReadHexColor("panel_ammo_shadow_color", PanelAmmoShadowColor);
    PanelAmmoShadowOffset = ReadFloat("panel_ammo_shadow_offset", PanelAmmoShadowOffset);

    PanelVitalsOffsetY = ReadFloat("panel_vitals_offset_y", PanelVitalsOffsetY);
    PanelVitalsGap = ReadFloat("panel_vitals_gap", PanelVitalsGap);
    PanelVitalsLength = ReadFloat("panel_vitals_length", PanelVitalsLength);
    PanelVitalsThicknessHealth = ReadFloat("panel_vitals_thickness_health", PanelVitalsThicknessHealth);
    PanelVitalsThicknessArmor = ReadFloat("panel_vitals_thickness_armor", PanelVitalsThicknessArmor);
    PanelVitalsThicknessStamina = ReadFloat("panel_vitals_thickness_stamina", PanelVitalsThicknessStamina);
    PanelVitalsHealthColor = ReadHexColor("panel_vitals_health_color", PanelVitalsHealthColor);
    PanelVitalsHealthTrackColor = ReadHexColor("panel_vitals_health_track_color", PanelVitalsHealthTrackColor);
    PanelVitalsArmorColor = ReadHexColor("panel_vitals_armor_color", PanelVitalsArmorColor);
    PanelVitalsStaminaColor = ReadHexColor("panel_vitals_stamina_color", PanelVitalsStaminaColor);
    PanelVitalsBreathColor = ReadHexColor("panel_vitals_breath_color", PanelVitalsBreathColor);
    PanelVitalsBreathHold = ReadFloat("panel_vitals_breath_hold", PanelVitalsBreathHold);
    PanelVitalsCursorColor = ReadHexColor("panel_vitals_cursor_color", PanelVitalsCursorColor);
    PanelVitalsCursorWidth = ReadFloat("panel_vitals_cursor_width", PanelVitalsCursorWidth);
    PanelVitalsCursorScale = ReadFloat("panel_vitals_cursor_scale", PanelVitalsCursorScale);
    PanelVitalsArmorUnit = ReadFloat("panel_vitals_armor_unit", PanelVitalsArmorUnit);
    PanelVitalsArmorGap = ReadFloat("panel_vitals_armor_gap", PanelVitalsArmorGap);
    PanelVitalsStaminaFadeIn = ReadFloat("panel_vitals_stamina_fade_in", PanelVitalsStaminaFadeIn);
    PanelVitalsStaminaFadeOut = ReadFloat("panel_vitals_stamina_fade_out", PanelVitalsStaminaFadeOut);
    PanelVitalsStaminaHold = ReadFloat("panel_vitals_stamina_hold", PanelVitalsStaminaHold);

    PanelHudTextSize = ReadFloat("panel_hud_text_size", PanelHudTextSize);
    PanelHudIconSize = ReadFloat("panel_hud_icon_size", PanelHudIconSize);
    PanelHudTextColor = ReadHexColor("panel_hud_text_color", PanelHudTextColor);
    PanelHudIconColor = ReadHexColor("panel_hud_icon_color", PanelHudIconColor);
    PanelMoneyDigits = ReadInt("panel_money_digits", PanelMoneyDigits);
    PanelSkillIconX = ReadFloat("panel_skill_icon_x", PanelSkillIconX);
    PanelSkillValueX = ReadFloat("panel_skill_value_x", PanelSkillValueX);
    PanelClockIconX = ReadFloat("panel_clock_icon_x", PanelClockIconX);
    PanelClockValueX = ReadFloat("panel_clock_value_x", PanelClockValueX);
    PanelWantedIconX = ReadFloat("panel_wanted_icon_x", PanelWantedIconX);
    PanelWantedValueX = ReadFloat("panel_wanted_value_x", PanelWantedValueX);
    PanelLineWidth = ReadFloat("panel_line_width", PanelLineWidth);
    PanelLineAlpha = ReadFloat("panel_line_alpha", PanelLineAlpha);
    PanelLineAlphaBottom = ReadFloat("panel_line_alpha_bottom", PanelLineAlphaBottom);
    PanelAdsDuration = ReadFloat("panel_ads_duration", PanelAdsDuration);
    PanelAdsWidth = ReadFloat("panel_ads_width", PanelAdsWidth);
    PanelAdsCenterX = ReadFloat("panel_ads_center_x", PanelAdsCenterX);
    PanelAdsCenterY = ReadFloat("panel_ads_center_y", PanelAdsCenterY);
    PanelAdsYaw = ReadFloat("panel_ads_yaw", PanelAdsYaw);
    PanelAdsPitch = ReadFloat("panel_ads_pitch", PanelAdsPitch);

    BaseYawOffset = ReadFloat("base_yaw_offset", BaseYawOffset);
    BasePitch = ReadFloat("base_pitch", BasePitch);
    PitchDeadzone = ReadFloat("pitch_deadzone", PitchDeadzone);
    PitchSmooth = ReadFloat("pitch_smooth", PitchSmooth);
    YawSmooth = ReadFloat("yaw_smooth", YawSmooth);
    SideSmooth = ReadFloat("side_smooth", SideSmooth);

    SpringOmega = ReadFloat("spring_omega", SpringOmega);
    AnchorSmooth = ReadFloat("anchor_smooth", AnchorSmooth);
    BobAmplitude = ReadFloat("bob_amplitude", BobAmplitude);
    BobOmega = ReadFloat("bob_omega", BobOmega);
    TurnTiltGain = ReadFloat("turn_tilt_gain", TurnTiltGain);
    TurnTiltMax = ReadFloat("turn_tilt_max", TurnTiltMax);

    FadeInSpeed = ReadFloat("fade_in_speed", FadeInSpeed);
    FadeOutSpeed = ReadFloat("fade_out_speed", FadeOutSpeed);
    FadeNearDist = ReadFloat("fade_near_dist", FadeNearDist);
    FadeNearFull = ReadFloat("fade_near_full", FadeNearFull);

    BlurSpacing = ReadFloat("blur_spacing", BlurSpacing);
    BlurIterations = ReadInt("blur_iterations", BlurIterations);
    BlurSaturation = ReadFloat("blur_saturation", BlurSaturation);

    BackTint = ReadHexColor("back_tint", BackTint);
}

void Update()
{
    if ((g_frameCounter++ % 30) != 0)
        return;

    WIN32_FILE_ATTRIBUTE_DATA attributes { };
    if (!GetFileAttributesExA(IniPath().c_str(), GetFileExInfoStandard, &attributes))
        return;

    ULONGLONG const last_write = (static_cast<ULONGLONG>(attributes.ftLastWriteTime.dwHighDateTime) << 32) | static_cast<ULONGLONG>(attributes.ftLastWriteTime.dwLowDateTime);

    if (last_write != g_lastWrite) {
        g_lastWrite = last_write;
        Load();
    }
}
}
