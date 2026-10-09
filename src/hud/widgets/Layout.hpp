#pragma once

#include <algorithm>

#include "core/Config.hpp"

namespace division::hud {

struct Rect
{
    float x0, y0, x1, y1;
};

struct PanelRects
{
    Rect ammo;
    Rect skill;
    Rect resource;
};

struct AmmoZones
{
    Rect clip;
    Rect reserve;
    Rect prev;
    Rect next;
    float divider_x;
};

// The card's on-screen width / height aspect; geometry measured in normalized card
// units needs it to stay square when drawn.
inline float UnitAspect()
{
    return core::cfg.PanelHeight / std::max(core::cfg.PanelLength, 0.0001f);
}

inline AmmoZones ComputeAmmoZones(Rect const& ammo, float split, float clip_split)
{
    float const width = ammo.x1 - ammo.x0;
    float const divider_x = ammo.x0 + width * std::clamp(split, 0.05f, 0.95f);

    Rect const left = { ammo.x0, ammo.y0, divider_x, ammo.y1 };
    Rect const right = { divider_x, ammo.y0, ammo.x1, ammo.y1 };
    float const clip_y = left.y1 - (left.y1 - left.y0) * std::clamp(clip_split, 0.1f, 0.9f);
    float const right_mid = (right.y0 + right.y1) * 0.5f;

    AmmoZones zones { };
    zones.divider_x = divider_x;
    zones.clip = { left.x0, clip_y, left.x1, left.y1 };
    zones.reserve = { left.x0, left.y0, left.x1, clip_y };
    zones.prev = { right.x0, right_mid, right.x1, right.y1 };
    zones.next = { right.x0, right.y0, right.x1, right_mid };
    return zones;
}

inline PanelRects Compute(float column_length, float skill_height, float gap)
{
    column_length = std::clamp(column_length, 0.01f, 0.5f);
    skill_height = std::clamp(skill_height, 0.01f, 0.5f);
    gap = std::clamp(gap, 0.0f, 1.0f);

    float right_width = 1.0f - column_length - gap;
    if (right_width < 0.01f)
        right_width = 0.01f;

    PanelRects rects { };
    rects.ammo = { 0.0f, 0.0f, column_length, 1.0f };

    float const right_x0 = column_length + gap;
    rects.skill = { right_x0, 1.0f - skill_height, right_x0 + right_width, 1.0f };
    rects.resource = { right_x0, 0.0f, right_x0 + right_width, skill_height };
    return rects;
}

}