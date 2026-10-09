#include "core/Config.hpp"
#include "plugin.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace division::core {
namespace {
    // All legacy releases stored every key flat under [DivisionHud4SA]; the fallback
    // keeps reading those so an existing user file keeps its values until they are
    // migrated. New keys are read first and win if present.
    char const LEGACY_SECTION[] = "DivisionHud4SA";

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

    bool ReadRaw(char const* section, char const* key, char const* legacy_key, char (&buffer)[64])
    {
        GetPrivateProfileStringA(section, key, "", buffer, sizeof(buffer), IniPath().c_str());
        if (buffer[0] == '\0' && legacy_key)
            GetPrivateProfileStringA(LEGACY_SECTION, legacy_key, "", buffer, sizeof(buffer), IniPath().c_str());
        return buffer[0] != '\0';
    }

    // Single source of truth for every config key: INI section/key, optional legacy
    // key, the storage type and the Config member it maps to. Load() and WriteDefaults()
    // both drive off this table, so a key can never be added/removed in only one place.
    struct Entry {
        char const* section;
        char const* key;
        char const* legacy_key;
        char kind;          // 'f' float, 'c' D3DCOLOR, 'i' int
        std::size_t offset; // offsetof(Config, ..)
    };

#define F(section, key, legacy, member) { section, key, legacy, 'f', offsetof(Config, member) }
#define C(section, key, legacy, member) { section, key, legacy, 'c', offsetof(Config, member) }
#define I(section, key, legacy, member) { section, key, legacy, 'i', offsetof(Config, member) }

    constexpr Entry kEntries[] = {
        // [anchor]  Card position relative to the player's feet/orientation.
        F("anchor", "offset_right", "anchor_offset_right", AnchorOffsetRight),
        F("anchor", "offset_up", "anchor_offset_up", AnchorOffsetUp),
        F("anchor", "offset_forward", "anchor_offset_forward", AnchorOffsetForward),

        // [follow]  How the card tracks the camera/player.
        F("follow", "bone_influence", "bone_influence", BoneInfluence),
        F("follow", "spring_omega", "spring_omega", SpringOmega),
        F("follow", "anchor_smooth", "anchor_smooth", AnchorSmooth),
        F("follow", "side_smooth", "side_smooth", SideSmooth),
        F("follow", "yaw_smooth", "yaw_smooth", YawSmooth),
        F("follow", "pitch_smooth", "pitch_smooth", PitchSmooth),
        F("follow", "pitch_deadzone", "pitch_deadzone", PitchDeadzone),
        F("follow", "base_yaw_offset", "base_yaw_offset", BaseYawOffset),
        F("follow", "base_pitch", "base_pitch", BasePitch),
        F("follow", "bob_amplitude", "bob_amplitude", BobAmplitude),
        F("follow", "bob_omega", "bob_omega", BobOmega),
        F("follow", "turn_tilt_gain", "turn_tilt_gain", TurnTiltGain),
        F("follow", "turn_tilt_max", "turn_tilt_max", TurnTiltMax),
        F("follow", "fade_in_speed", "fade_in_speed", FadeInSpeed),
        F("follow", "fade_out_speed", "fade_out_speed", FadeOutSpeed),
        F("follow", "fade_near_dist", "fade_near_dist", FadeNearDist),
        F("follow", "fade_near_full", "fade_near_full", FadeNearFull),

        // [panel]  Card proportions and outline.
        F("panel", "length", "panel_length", PanelLength),
        F("panel", "height", "panel_height", PanelHeight),
        F("panel", "column_length", "panel_column_length", PanelColumnLength),
        F("panel", "skill_height", "panel_skill_height", PanelSkillHeight),
        F("panel", "gap", "panel_gap", PanelGap),
        F("panel", "line_width", "panel_line_width", PanelLineWidth),
        F("panel", "line_alpha", "panel_line_alpha", PanelLineAlpha),
        F("panel", "line_alpha_bottom", "panel_line_alpha_bottom", PanelLineAlphaBottom),

        // [ads]  Aim-down-sights: card blended onto a fixed screen-space quad.
        F("ads", "duration", "panel_ads_duration", PanelAdsDuration),
        F("ads", "width", "panel_ads_width", PanelAdsWidth),
        F("ads", "center_x", "panel_ads_center_x", PanelAdsCenterX),
        F("ads", "center_y", "panel_ads_center_y", PanelAdsCenterY),
        F("ads", "yaw", "panel_ads_yaw", PanelAdsYaw),
        F("ads", "pitch", "panel_ads_pitch", PanelAdsPitch),

        // [ammo]  Magazine / reserve / prev / next numbers on the left card.
        F("ammo", "split", "panel_ammo_split", PanelAmmoSplit),
        F("ammo", "clip_split", "panel_ammo_clip_split", PanelAmmoClipSplit),
        F("ammo", "clip_size", "panel_ammo_clip_size", PanelAmmoClipSize),
        F("ammo", "reserve_size", "panel_ammo_reserve_size", PanelAmmoReserveSize),
        F("ammo", "side_size", "panel_ammo_side_size", PanelAmmoSideSize),
        F("ammo", "clip_x", "panel_ammo_clip_x", PanelAmmoClipOffsetX),
        F("ammo", "clip_y", "panel_ammo_clip_y", PanelAmmoClipOffsetY),
        F("ammo", "reserve_x", "panel_ammo_reserve_x", PanelAmmoReserveOffsetX),
        F("ammo", "reserve_y", "panel_ammo_reserve_y", PanelAmmoReserveOffsetY),
        F("ammo", "prev_x", "panel_ammo_prev_x", PanelAmmoPrevOffsetX),
        F("ammo", "prev_y", "panel_ammo_prev_y", PanelAmmoPrevOffsetY),
        F("ammo", "next_x", "panel_ammo_next_x", PanelAmmoNextOffsetX),
        F("ammo", "next_y", "panel_ammo_next_y", PanelAmmoNextOffsetY),
        C("ammo", "clip_color", "panel_ammo_clip_color", PanelAmmoClipColor),
        C("ammo", "reserve_color", "panel_ammo_reserve_color", PanelAmmoReserveColor),
        C("ammo", "side_color", "panel_ammo_side_color", PanelAmmoSideColor),
        C("ammo", "shadow_color", "panel_ammo_shadow_color", PanelAmmoShadowColor),
        F("ammo", "shadow_offset", "panel_ammo_shadow_offset", PanelAmmoShadowOffset),
        F("ammo", "clip_pop", "panel_ammo_clip_pop", PanelAmmoClipPop),
        F("ammo", "fire_pop", "panel_ammo_fire_pop", PanelAmmoFirePop),
        F("ammo", "fire_pop_decay", "panel_ammo_fire_pop_decay", PanelAmmoFirePopDecay),
        F("ammo", "reload_transition", "panel_ammo_reload_transition", PanelAmmoReloadTransition),
        C("ammo", "side_reload_color", "panel_ammo_side_reload_color", PanelAmmoSideReloadColor),

        // [weapon_preview]  Weapon-switch preview row (screen-space, top-centre).
        F("weapon_preview", "center_x", "weapon_preview_center_x", WeaponPreviewCenterX),
        F("weapon_preview", "center_y", "weapon_preview_center_y", WeaponPreviewCenterY),
        I("weapon_preview", "max_count", "weapon_preview_max_count", WeaponPreviewMaxCount),
        F("weapon_preview", "icon_height", "weapon_preview_icon_height", WeaponPreviewIconHeight),
        F("weapon_preview", "panel_aspect", "weapon_preview_panel_aspect", WeaponPreviewPanelAspect),
        F("weapon_preview", "icon_pad", "weapon_preview_icon_pad", WeaponPreviewIconPad),
        F("weapon_preview", "ammo_ratio", "weapon_preview_ammo_ratio", WeaponPreviewAmmoRatio),
        F("weapon_preview", "gap", "weapon_preview_gap", WeaponPreviewGap),
        F("weapon_preview", "max_width", "weapon_preview_max_width", WeaponPreviewMaxWidth),
        F("weapon_preview", "active_scale", "weapon_preview_active_scale", WeaponPreviewActiveScale),
        C("weapon_preview", "active_color", "weapon_preview_active_color", WeaponPreviewActiveColor),
        C("weapon_preview", "line_color", "weapon_preview_line_color", WeaponPreviewLineColor),
        F("weapon_preview", "line_width", "weapon_preview_line_width", WeaponPreviewLineWidth),
        C("weapon_preview", "text_color", "weapon_preview_text_color", WeaponPreviewTextColor),
        F("weapon_preview", "text_size", "weapon_preview_text_size", WeaponPreviewTextSize),
        F("weapon_preview", "hold", "weapon_preview_hold", WeaponPreviewHold),
        F("weapon_preview", "fade_in", "weapon_preview_fade_in", WeaponPreviewFadeIn),
        F("weapon_preview", "fade_out", "weapon_preview_fade_out", WeaponPreviewFadeOut),
        F("weapon_preview", "slide_speed", "weapon_preview_slide_speed", WeaponPreviewSlideSpeed),

        // [vitals]  Health / armour / stamina / breath bars above the card.
        F("vitals", "offset_y", "panel_vitals_offset_y", PanelVitalsOffsetY),
        F("vitals", "gap", "panel_vitals_gap", PanelVitalsGap),
        F("vitals", "length", "panel_vitals_length", PanelVitalsLength),
        F("vitals", "thickness_health", "panel_vitals_thickness_health", PanelVitalsThicknessHealth),
        F("vitals", "thickness_armor", "panel_vitals_thickness_armor", PanelVitalsThicknessArmor),
        F("vitals", "thickness_stamina", "panel_vitals_thickness_stamina", PanelVitalsThicknessStamina),
        C("vitals", "health_color", "panel_vitals_health_color", PanelVitalsHealthColor),
        C("vitals", "health_track_color", "panel_vitals_health_track_color", PanelVitalsHealthTrackColor),
        C("vitals", "armor_color", "panel_vitals_armor_color", PanelVitalsArmorColor),
        C("vitals", "stamina_color", "panel_vitals_stamina_color", PanelVitalsStaminaColor),
        C("vitals", "breath_color", "panel_vitals_breath_color", PanelVitalsBreathColor),
        F("vitals", "breath_hold", "panel_vitals_breath_hold", PanelVitalsBreathHold),
        C("vitals", "cursor_color", "panel_vitals_cursor_color", PanelVitalsCursorColor),
        F("vitals", "cursor_width", "panel_vitals_cursor_width", PanelVitalsCursorWidth),
        F("vitals", "cursor_scale", "panel_vitals_cursor_scale", PanelVitalsCursorScale),
        F("vitals", "cursor_dot_gap", "panel_vitals_cursor_dot_gap", PanelVitalsCursorDotGap),
        F("vitals", "armor_unit", "panel_vitals_armor_unit", PanelVitalsArmorUnit),
        F("vitals", "armor_gap", "panel_vitals_armor_gap", PanelVitalsArmorGap),
        F("vitals", "stamina_fade_in", "panel_vitals_stamina_fade_in", PanelVitalsStaminaFadeIn),
        F("vitals", "stamina_fade_out", "panel_vitals_stamina_fade_out", PanelVitalsStaminaFadeOut),
        F("vitals", "stamina_hold", "panel_vitals_stamina_hold", PanelVitalsStaminaHold),
        F("vitals", "move_fade", "panel_vitals_move_fade", PanelVitalsMoveFade),
        C("vitals", "health_chip_color", "panel_vitals_health_chip_color", PanelVitalsHealthChipColor),
        F("vitals", "health_chip_hold", "panel_vitals_health_chip_hold", PanelVitalsHealthChipHold),
        F("vitals", "health_chip_speed", "panel_vitals_health_chip_speed", PanelVitalsHealthChipSpeed),
        F("vitals", "health_pulse_hold", "panel_vitals_health_pulse_hold", PanelVitalsHealthPulseHold),
        F("vitals", "health_pulse_scale", "panel_vitals_health_pulse_scale", PanelVitalsHealthPulseScale),
        F("vitals", "health_pulse_fade_in", "panel_vitals_health_pulse_fade_in", PanelVitalsHealthPulseFadeIn),
        F("vitals", "health_pulse_fade_out", "panel_vitals_health_pulse_fade_out", PanelVitalsHealthPulseFadeOut),

        // [damage]  Damage-source direction indicators around the crosshair.
        F("damage", "center_x", "panel_damage_center_x", PanelDamageCenterX),
        F("damage", "center_y", "panel_damage_center_y", PanelDamageCenterY),
        F("damage", "radius", "panel_damage_radius", PanelDamageRadius),
        F("damage", "arc_thickness", "panel_damage_arc_thickness", PanelDamageArcThickness),
        F("damage", "arc_max_deg", "panel_damage_arc_max_deg", PanelDamageArcMaxDeg),
        F("damage", "arc_min_deg", "panel_damage_arc_min_deg", PanelDamageArcMinDeg),
        F("damage", "arc_gain_deg", "panel_damage_arc_gain_deg", PanelDamageArcGainDeg),
        F("damage", "arc_hold", "panel_damage_arc_hold", PanelDamageArcHold),
        F("damage", "arc_shrink_deg", "panel_damage_arc_shrink_deg", PanelDamageArcShrinkDeg),
        F("damage", "forget", "panel_damage_forget", PanelDamageForget),
        F("damage", "pointer_size", "panel_damage_pointer_size", PanelDamagePointerSize),
        F("damage", "pointer_width", "panel_damage_pointer_width", PanelDamagePointerWidth),
        C("damage", "color_armor", "panel_damage_color_armor", PanelDamageColorArmor),
        C("damage", "color_health", "panel_damage_color_health", PanelDamageColorHealth),
        I("damage", "max_sources", "panel_damage_max_sources", PanelDamageMaxSources),

        // [hud]  Info panel text/icons (money, clock, wanted count).
        F("hud", "text_size", "panel_hud_text_size", PanelHudTextSize),
        F("hud", "icon_size", "panel_hud_icon_size", PanelHudIconSize),
        C("hud", "text_color", "panel_hud_text_color", PanelHudTextColor),
        C("hud", "icon_color", "panel_hud_icon_color", PanelHudIconColor),
        C("hud", "text_shadow_color", "panel_hud_text_shadow_color", PanelHudTextShadowColor),
        F("hud", "text_shadow_offset", "panel_hud_text_shadow_offset", PanelHudTextShadowOffset),
        I("hud", "money_digits", "panel_money_digits", PanelMoneyDigits),
        F("hud", "skill_icon_x", "panel_skill_icon_x", PanelSkillIconX),
        F("hud", "skill_value_x", "panel_skill_value_x", PanelSkillValueX),
        F("hud", "clock_icon_x", "panel_clock_icon_x", PanelClockIconX),
        F("hud", "clock_value_x", "panel_clock_value_x", PanelClockValueX),
        F("hud", "wanted_icon_x", "panel_wanted_icon_x", PanelWantedIconX),
        F("hud", "wanted_value_x", "panel_wanted_value_x", PanelWantedValueX),

        // [wanted]  Wanted-tier progress overlay on the resource card.
        F("wanted", "bar_x0", "panel_wanted_bar_x0", PanelWantedBarX0),
        F("wanted", "bar_x1", "panel_wanted_bar_x1", PanelWantedBarX1),
        C("wanted", "bar_color", "panel_wanted_bar_color", PanelWantedBarColor),
        F("wanted", "bar_speed", "panel_wanted_bar_speed", PanelWantedBarSpeed),
        F("wanted", "breath", "panel_wanted_breath", PanelWantedBreath),
        C("wanted", "wave_color", "panel_wanted_wave_color", PanelWantedWaveColor),
        F("wanted", "wave_width", "panel_wanted_wave_width", PanelWantedWaveWidth),
        F("wanted", "wave_sweep", "panel_wanted_wave_sweep", PanelWantedWaveSweep),
        F("wanted", "wave_fade", "panel_wanted_wave_fade", PanelWantedWaveFade),
        F("wanted", "wave_interval", "panel_wanted_wave_interval", PanelWantedWaveInterval),
        C("wanted", "full_color", "panel_wanted_full_color", PanelWantedFullColor),
        F("wanted", "full_duration", "panel_wanted_full_duration", PanelWantedFullDuration),

        // [blur]  Shared full-screen blur behind the cards.
        F("blur", "spacing", "blur_spacing", BlurSpacing),
        I("blur", "iterations", "blur_iterations", BlurIterations),
        F("blur", "saturation", "blur_saturation", BlurSaturation),
    };

#undef F
#undef C
#undef I

    void* Member(Config& cfg, Entry const& e)
    {
        return reinterpret_cast<unsigned char*>(&cfg) + e.offset;
    }

    void const* Member(Config const& cfg, Entry const& e)
    {
        return reinterpret_cast<unsigned char const*>(&cfg) + e.offset;
    }

    void LoadEntry(Config& cfg, Entry const& e)
    {
        char buffer[64] { };
        if (!ReadRaw(e.section, e.key, e.legacy_key, buffer)) return;

        char* end = nullptr;
        switch (e.kind) {
            case 'f': {
                // strtof verifies the whole token parses; garbage or trailing text keeps
                // the previous value instead of silently snapping to 0.
                float const value = std::strtof(buffer, &end);
                if (end != buffer && *end == '\0')
                    *static_cast<float*>(Member(cfg, e)) = value;
                break;
            }
            case 'c': {
                unsigned long const value = std::strtoul(buffer, &end, 16);
                if (end != buffer && *end == '\0')
                    *static_cast<D3DCOLOR*>(Member(cfg, e)) = static_cast<D3DCOLOR>(value);
                break;
            }
            case 'i': {
                long const value = std::strtol(buffer, &end, 10);
                if (end != buffer && *end == '\0')
                    *static_cast<int*>(Member(cfg, e)) = static_cast<int>(value);
                break;
            }
        }
    }

    std::string FormatValue(Config const& cfg, Entry const& e)
    {
        char buffer[32];
        void const* const member = Member(cfg, e);
        switch (e.kind) {
            case 'f':
                std::snprintf(buffer, sizeof(buffer), "%g", *static_cast<float const*>(member));
                break;
            case 'c':
                std::snprintf(buffer, sizeof(buffer), "%08X", static_cast<unsigned int>(*static_cast<D3DCOLOR const*>(member)));
                break;
            case 'i':
                std::snprintf(buffer, sizeof(buffer), "%d", *static_cast<int const*>(member));
                break;
            default:
                return {};
        }
        return buffer;
    }

    void WriteFileContents(std::string const& contents)
    {
        HANDLE const file = CreateFileA(IniPath().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;

        DWORD written = 0;
        WriteFile(file, contents.data(), static_cast<DWORD>(contents.size()), &written, nullptr);
        CloseHandle(file);
    }

    void WriteDefaults()
    {
        CreateDirectoryA(PLUGIN_PATH("DivisionHud4SA"), nullptr);
        CreateDirectoryA(PLUGIN_PATH("DivisionHud4SA/config"), nullptr);

        std::string contents;
        char const* current_section = nullptr;
        for (Entry const& e : kEntries) {
            if (current_section == nullptr || std::strcmp(current_section, e.section) != 0) {
                contents += '[';
                contents += e.section;
                contents += "]\r\n";
                current_section = e.section;
            }
            contents += e.key;
            contents += " = ";
            contents += FormatValue(cfg, e);
            contents += "\r\n";
        }

        WriteFileContents(contents);
    }
}

void Load()
{
    if (GetFileAttributesA(IniPath().c_str()) == INVALID_FILE_ATTRIBUTES)
        WriteDefaults();

    for (Entry const& e : kEntries)
        LoadEntry(cfg, e);
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