#pragma once

#include <d3d9.h>
#include <vector>

#include "render/Surface.hpp"

namespace division::render {

D3DCOLOR ScaleColor(D3DCOLOR color, float extra);

void DrawTexturedQuad(IDirect3DDevice9* device, ScreenPoint const (&points)[4], float const (&uv)[4][2],
    D3DCOLOR color);

// Draws the panel's blurred backdrop. The blur texture's own colour is used verbatim
// (D3DTOP_SELECTARG1) so nothing is tinted or multiplied in; `opacity` (0..1) is the
// only parameter and just controls how much the backdrop covers the scene behind it.
void DrawBackdrop(IDirect3DDevice9* device, ScreenPoint const (&points)[4], float const (&uv)[4][2],
    float opacity);

void DrawLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to, float thickness,
    D3DCOLOR color);
void DrawEdgeLine(IDirect3DDevice9* device, ScreenPoint const& from, ScreenPoint const& to,
    ScreenPoint const& interior, float thickness, D3DCOLOR color);

// Anti-aliased triangle (soft edges via the bar shader's edge-distance coverage).
void AppendAATri(std::vector<BarVertex>& out, ScreenPoint const& a, ScreenPoint const& b,
    ScreenPoint const& c, D3DCOLOR color);
void AppendAARect(std::vector<BarVertex>& out, PanelSurface const& surface, float x0, float y0, float x1,
    float y1, D3DCOLOR color);
void AppendRing(std::vector<BarVertex>& out, PanelSurface const& surface, float cx, float cy, float rx,
    float ry, float inner_ratio, D3DCOLOR color);
void AppendStar(std::vector<BarVertex>& out, PanelSurface const& surface, float cx, float cy, float rx,
    float ry, D3DCOLOR color);

// Screen-space helpers for the damage indicators. Angles are measured clockwise from screen-up,
// i.e. direction(angle) = (sin angle, -cos angle).
void AppendArcScreen(std::vector<BarVertex>& out, float cx, float cy, float radius, float thickness,
    float center_angle, float half_angle, D3DCOLOR color);
void AppendPointerScreen(std::vector<BarVertex>& out, float cx, float cy, float radius, float angle,
    float size, float width_ratio, D3DCOLOR color);

}