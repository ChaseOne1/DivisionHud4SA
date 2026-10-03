#include "core/Config.hpp"
#include "plugin.h"

#include <cstdlib>
#include <string>

namespace division::core {
namespace {
    char const SECTION[] = "DivisionHud4SA";

    std::string g_path;
    unsigned long long g_last_write = 0;
    unsigned int g_frame_counter = 0;

    std::string const& IniPath()
    {
        if (g_path.empty())
            g_path = PLUGIN_PATH("DivisionHud4SA/config/DivisionHud4SA.ini");
        return g_path;
    }

    unsigned long long LastWriteTime(char const* path)
    {
        WIN32_FILE_ATTRIBUTE_DATA data { };
        if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
            return 0;
        return (static_cast<unsigned long long>(data.ftLastWriteTime.dwHighDateTime) << 32)
            | static_cast<unsigned long long>(data.ftLastWriteTime.dwLowDateTime);
    }

    float ReadFloat(char const* key, float default_value)
    {
        char buffer[64] { };
        GetPrivateProfileStringA(SECTION, key, "", buffer, sizeof(buffer), IniPath().c_str());
        if (buffer[0] == '\0') return default_value;
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
        if (buffer[0] == '\0') return default_value;
        return static_cast<D3DCOLOR>(std::strtoul(buffer, nullptr, 16));
    }

    void WriteDefaults()
    {
        static constexpr char text[] =
            "[DivisionHud4SA]\n"
                "anchor_offset_right = -1.00\n"
                "anchor_offset_up = 0.15\n"
                "anchor_offset_forward = 0.15\n"
                "bone_influence = 0.3\n"
                "panel_length = 0.80\n"
                "panel_height = 0.28\n"
                "panel_column_length = 0.48\n"
                "panel_skill_height = 0.49\n"
                "panel_gap = 0.01\n"
                "panel_ammo_split = 0.65\n"
                "panel_ammo_clip_split = 0.60\n"
                "panel_ammo_clip_size = 1.1\n"
                "panel_ammo_reserve_size = 1.1\n"
                "panel_ammo_side_size = 0.80\n"
                "panel_ammo_clip_x = 0.0\n"
                "panel_ammo_clip_y = 0.0\n"
                "panel_ammo_reserve_x = 0.0\n"
                "panel_ammo_reserve_y = 0.05\n"
                "panel_ammo_prev_x = 0.0\n"
                "panel_ammo_prev_y = 0.0\n"
                "panel_ammo_next_x = 0.0\n"
                "panel_ammo_next_y = 0.0\n"
                "panel_ammo_clip_color = FFFFFFFF\n"
                "panel_ammo_reserve_color = 77FFFFFF\n"
                "panel_ammo_side_color = CCFFFFFF\n"
                "panel_ammo_shadow_color = 44000000\n"
                "panel_ammo_shadow_offset = 2.0\n"
                "panel_ammo_clip_pop = 0.10\n"
                "panel_ammo_fire_pop = 0.06\n"
                "panel_ammo_fire_pop_decay = 6.0\n"
                "panel_ammo_reload_transition = 0.25\n"
                "panel_ammo_side_reload_color = 55FFFFFF\n"
                "weapon_preview_center_x = 0.5\n"
                "weapon_preview_center_y = 0.16\n"
                "weapon_preview_max_count = 5\n"
                "weapon_preview_icon_height = 0.05\n"
                "weapon_preview_panel_aspect = 2.4\n"
                "weapon_preview_icon_pad = 0.18\n"
                "weapon_preview_ammo_ratio = 0.55\n"
                "weapon_preview_gap = 0.006\n"
                "weapon_preview_max_width = 0.72\n"
                "weapon_preview_active_scale = 1.12\n"
                "weapon_preview_active_color = FFE8871A\n"
                "weapon_preview_line_color = CCFFFFFF\n"
                "weapon_preview_line_width = 1.0\n"
                "weapon_preview_text_color = FFFFFFFF\n"
                "weapon_preview_text_size = 0.75\n"
                "weapon_preview_hold = 1.5\n"
                "weapon_preview_fade_in = 0.15\n"
                "weapon_preview_fade_out = 0.30\n"
                "weapon_preview_slide_speed = 4.0\n"
                "weapon_preview_scale_speed = 8.0\n"
                "panel_vitals_offset_y = 0.04\n"
                "panel_vitals_gap = 0.05\n"
                "panel_vitals_length = 1.0\n"
                "panel_vitals_thickness_health = 0.06\n"
                "panel_vitals_thickness_armor = 0.15\n"
                "panel_vitals_thickness_stamina = 0.05\n"
                "panel_vitals_health_color = FFE8871A\n"
                "panel_vitals_health_track_color = 44000000\n"
                "panel_vitals_armor_color = FFFFFFFF\n"
                "panel_vitals_stamina_color = FFFFFFFF\n"
                "panel_vitals_breath_color = FF2E9BFF\n"
                "panel_vitals_breath_hold = 1.0\n"
                "panel_vitals_cursor_color = FFFFFFFF\n"
                "panel_vitals_cursor_width = 0.004\n"
                "panel_vitals_cursor_scale = 1.6\n"
                "panel_vitals_cursor_dot_gap = 0.012\n"
                "panel_vitals_armor_unit = 3.0\n"
                "panel_vitals_armor_gap = 0.008\n"
                "panel_vitals_stamina_fade_in = 0.09\n"
                "panel_vitals_stamina_fade_out = 0.09\n"
                "panel_vitals_stamina_hold = 1.0\n"
                "panel_vitals_move_fade = 0.18\n"
                "panel_vitals_health_chip_color = FFD22D2D\n"
                "panel_vitals_health_chip_hold = 0.5\n"
                "panel_vitals_health_chip_speed = 0.8\n"
                "panel_vitals_health_pulse_hold = 0.15\n"
                "panel_vitals_health_pulse_scale = 0.6\n"
                "panel_vitals_health_pulse_fade_in = 0.05\n"
                "panel_vitals_health_pulse_fade_out = 0.25\n"
                "panel_damage_center_x = 0.5\n"
                "panel_damage_center_y = 0.5\n"
                "panel_damage_radius = 90.0\n"
                "panel_damage_arc_thickness = 5.0\n"
                "panel_damage_arc_max_deg = 90.0\n"
                "panel_damage_arc_min_deg = 12.0\n"
                "panel_damage_arc_gain_deg = 10.0\n"
                "panel_damage_arc_hold = 0.6\n"
                "panel_damage_arc_shrink_deg = 60.0\n"
                "panel_damage_forget = 3.0\n"
                "panel_damage_pointer_size = 14.0\n"
                "panel_damage_pointer_width = 0.4\n"
                "panel_damage_color_armor = FFFFFFFF\n"
                "panel_damage_color_health = FFE22D2D\n"
                "panel_damage_max_sources = 8\n"
                "panel_hud_text_size = 0.6\n"
                "panel_hud_icon_size = 0.5\n"
                "panel_hud_text_color = FFFFFFFF\n"
                "panel_hud_icon_color = FFFFFFFF\n"
                "panel_hud_text_shadow_color = 44000000\n"
                "panel_hud_text_shadow_offset = 2.0\n"
                "panel_money_digits = 10\n"
                "panel_skill_icon_x = 0.05\n"
                "panel_skill_value_x = 0.17\n"
                "panel_clock_icon_x = 0.1\n"
                "panel_clock_value_x = 0.22\n"
                "panel_wanted_icon_x = 0.68\n"
                "panel_wanted_value_x = 0.82\n"
                "panel_line_width = 1.0\n"
                "panel_line_alpha = 0.8\n"
                "panel_line_alpha_bottom = 0.2\n"
                "panel_ads_duration = 0.11\n"
                "panel_ads_width = 0.17\n"
                "panel_ads_center_x = 0.68\n"
                "panel_ads_center_y = 0.70\n"
                "panel_ads_yaw = -0.05\n"
                "panel_ads_pitch = 0.05\n"
                "base_yaw_offset = -0.40\n"
                "base_pitch = -0.03\n"
                "pitch_deadzone = 0.8\n"
                "pitch_smooth = 450.0\n"
                "yaw_smooth = 450.0\n"
                "side_smooth = 400.0\n"
                "spring_omega = 182.0\n"
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
                "back_tint = FFFFFFFF\n";

        CreateDirectoryA(PLUGIN_PATH("DivisionHud4SA"), nullptr);
        CreateDirectoryA(PLUGIN_PATH("DivisionHud4SA/config"), nullptr);

        HANDLE const file = CreateFileA(IniPath().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;

        char buffer[4096] { };
        DWORD count = 0;
        for (DWORD i = 0; text[i] != '\0' && count + 1 < sizeof(buffer); ++i) {
            if (text[i] == '\n') buffer[count++] = '\r';
            buffer[count++] = text[i];
        }

        DWORD written = 0;
        WriteFile(file, buffer, count, &written, nullptr);
        CloseHandle(file);
    }
}

void Load()
{
    if (GetFileAttributesA(IniPath().c_str()) == INVALID_FILE_ATTRIBUTES)
        WriteDefaults();

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
    PanelAmmoClipPop = ReadFloat("panel_ammo_clip_pop", PanelAmmoClipPop);
    PanelAmmoFirePop = ReadFloat("panel_ammo_fire_pop", PanelAmmoFirePop);
    PanelAmmoFirePopDecay = ReadFloat("panel_ammo_fire_pop_decay", PanelAmmoFirePopDecay);
    PanelAmmoReloadTransition = ReadFloat("panel_ammo_reload_transition", PanelAmmoReloadTransition);
    PanelAmmoSideReloadColor = ReadHexColor("panel_ammo_side_reload_color", PanelAmmoSideReloadColor);

    WeaponPreviewCenterX = ReadFloat("weapon_preview_center_x", WeaponPreviewCenterX);
    WeaponPreviewCenterY = ReadFloat("weapon_preview_center_y", WeaponPreviewCenterY);
    WeaponPreviewMaxCount = ReadInt("weapon_preview_max_count", WeaponPreviewMaxCount);
    WeaponPreviewIconHeight = ReadFloat("weapon_preview_icon_height", WeaponPreviewIconHeight);
    WeaponPreviewPanelAspect = ReadFloat("weapon_preview_panel_aspect", WeaponPreviewPanelAspect);
    WeaponPreviewIconPad = ReadFloat("weapon_preview_icon_pad", WeaponPreviewIconPad);
    WeaponPreviewAmmoRatio = ReadFloat("weapon_preview_ammo_ratio", WeaponPreviewAmmoRatio);
    WeaponPreviewGap = ReadFloat("weapon_preview_gap", WeaponPreviewGap);
    WeaponPreviewMaxWidth = ReadFloat("weapon_preview_max_width", WeaponPreviewMaxWidth);
    WeaponPreviewActiveScale = ReadFloat("weapon_preview_active_scale", WeaponPreviewActiveScale);
    WeaponPreviewActiveColor = ReadHexColor("weapon_preview_active_color", WeaponPreviewActiveColor);
    WeaponPreviewLineColor = ReadHexColor("weapon_preview_line_color", WeaponPreviewLineColor);
    WeaponPreviewLineWidth = ReadFloat("weapon_preview_line_width", WeaponPreviewLineWidth);
    WeaponPreviewTextColor = ReadHexColor("weapon_preview_text_color", WeaponPreviewTextColor);
    WeaponPreviewTextSize = ReadFloat("weapon_preview_text_size", WeaponPreviewTextSize);
    WeaponPreviewHold = ReadFloat("weapon_preview_hold", WeaponPreviewHold);
    WeaponPreviewFadeIn = ReadFloat("weapon_preview_fade_in", WeaponPreviewFadeIn);
    WeaponPreviewFadeOut = ReadFloat("weapon_preview_fade_out", WeaponPreviewFadeOut);
    WeaponPreviewSlideSpeed = ReadFloat("weapon_preview_slide_speed", WeaponPreviewSlideSpeed);
    WeaponPreviewScaleSpeed = ReadFloat("weapon_preview_scale_speed", WeaponPreviewScaleSpeed);

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
    PanelVitalsCursorDotGap = ReadFloat("panel_vitals_cursor_dot_gap", PanelVitalsCursorDotGap);
    PanelVitalsArmorUnit = ReadFloat("panel_vitals_armor_unit", PanelVitalsArmorUnit);
    PanelVitalsArmorGap = ReadFloat("panel_vitals_armor_gap", PanelVitalsArmorGap);
    PanelVitalsStaminaFadeIn = ReadFloat("panel_vitals_stamina_fade_in", PanelVitalsStaminaFadeIn);
    PanelVitalsStaminaFadeOut = ReadFloat("panel_vitals_stamina_fade_out", PanelVitalsStaminaFadeOut);
    PanelVitalsStaminaHold = ReadFloat("panel_vitals_stamina_hold", PanelVitalsStaminaHold);
    PanelVitalsMoveFade = ReadFloat("panel_vitals_move_fade", PanelVitalsMoveFade);
    PanelVitalsHealthChipColor = ReadHexColor("panel_vitals_health_chip_color", PanelVitalsHealthChipColor);
    PanelVitalsHealthChipHold = ReadFloat("panel_vitals_health_chip_hold", PanelVitalsHealthChipHold);
    PanelVitalsHealthChipSpeed = ReadFloat("panel_vitals_health_chip_speed", PanelVitalsHealthChipSpeed);
    PanelVitalsHealthPulseHold = ReadFloat("panel_vitals_health_pulse_hold", PanelVitalsHealthPulseHold);
    PanelVitalsHealthPulseScale = ReadFloat("panel_vitals_health_pulse_scale", PanelVitalsHealthPulseScale);
    PanelVitalsHealthPulseFadeIn = ReadFloat("panel_vitals_health_pulse_fade_in", PanelVitalsHealthPulseFadeIn);
    PanelVitalsHealthPulseFadeOut = ReadFloat("panel_vitals_health_pulse_fade_out", PanelVitalsHealthPulseFadeOut);

    PanelDamageCenterX = ReadFloat("panel_damage_center_x", PanelDamageCenterX);
    PanelDamageCenterY = ReadFloat("panel_damage_center_y", PanelDamageCenterY);
    PanelDamageRadius = ReadFloat("panel_damage_radius", PanelDamageRadius);
    PanelDamageArcThickness = ReadFloat("panel_damage_arc_thickness", PanelDamageArcThickness);
    PanelDamageArcMaxDeg = ReadFloat("panel_damage_arc_max_deg", PanelDamageArcMaxDeg);
    PanelDamageArcMinDeg = ReadFloat("panel_damage_arc_min_deg", PanelDamageArcMinDeg);
    PanelDamageArcGainDeg = ReadFloat("panel_damage_arc_gain_deg", PanelDamageArcGainDeg);
    PanelDamageArcHold = ReadFloat("panel_damage_arc_hold", PanelDamageArcHold);
    PanelDamageArcShrinkDeg = ReadFloat("panel_damage_arc_shrink_deg", PanelDamageArcShrinkDeg);
    PanelDamageForget = ReadFloat("panel_damage_forget", PanelDamageForget);
    PanelDamagePointerSize = ReadFloat("panel_damage_pointer_size", PanelDamagePointerSize);
    PanelDamagePointerWidth = ReadFloat("panel_damage_pointer_width", PanelDamagePointerWidth);
    PanelDamageColorArmor = ReadHexColor("panel_damage_color_armor", PanelDamageColorArmor);
    PanelDamageColorHealth = ReadHexColor("panel_damage_color_health", PanelDamageColorHealth);
    PanelDamageMaxSources = ReadInt("panel_damage_max_sources", PanelDamageMaxSources);

    PanelHudTextSize = ReadFloat("panel_hud_text_size", PanelHudTextSize);
    PanelHudIconSize = ReadFloat("panel_hud_icon_size", PanelHudIconSize);
    PanelHudTextColor = ReadHexColor("panel_hud_text_color", PanelHudTextColor);
    PanelHudIconColor = ReadHexColor("panel_hud_icon_color", PanelHudIconColor);
    PanelHudTextShadowColor = ReadHexColor("panel_hud_text_shadow_color", PanelHudTextShadowColor);
    PanelHudTextShadowOffset = ReadFloat("panel_hud_text_shadow_offset", PanelHudTextShadowOffset);
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
    if ((g_frame_counter++ % 30) != 0) return;

    unsigned long long const last_write = LastWriteTime(IniPath().c_str());
    if (last_write == 0) return;

    if (last_write != g_last_write) {
        g_last_write = last_write;
        Load();
    }
}
}
