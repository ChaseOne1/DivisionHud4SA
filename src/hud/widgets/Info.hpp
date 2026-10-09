#pragma once

#include <d3d9.h>
#include <vector>

#include "render/Primitives.hpp"
#include "render/TextRenderer.hpp"
#include "hud/widgets/Layout.hpp"

namespace division::hud {

// Info panel icons (clock, wanted star) draw with the solid bar shader bound.
void DrawInfoIcons(IDirect3DDevice9* device, render::PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha, std::vector<render::BarVertex>& vertices);

// Info panel text (money, clock, wanted count) draws with the text render states bound.
void DrawInfoText(IDirect3DDevice9* device, render::TextRenderer& text, render::PanelSurface const& surface,
    PanelRects const& panels, float panel_alpha);

}