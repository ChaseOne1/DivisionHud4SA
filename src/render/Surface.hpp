#pragma once

#include <d3d9.h>

namespace division::render {
struct ScreenPoint
{
    float x = 0.0f;
    float y = 0.0f;
};

// Maps normalized local panel coordinates (x,y in [0,1], y up) onto a screen-space quad.
class PanelSurface
{
public:
    void SetCorners(ScreenPoint const& top_left, ScreenPoint const& top_right,
        ScreenPoint const& bottom_right, ScreenPoint const& bottom_left)
    {
        m_top_left = top_left;
        m_top_right = top_right;
        m_bottom_right = bottom_right;
        m_bottom_left = bottom_left;
    }

    ScreenPoint Map(float nx, float ny) const
    {
        float const bottom_x = m_bottom_left.x + (m_bottom_right.x - m_bottom_left.x) * nx;
        float const bottom_y = m_bottom_left.y + (m_bottom_right.y - m_bottom_left.y) * nx;
        float const top_x = m_top_left.x + (m_top_right.x - m_top_left.x) * nx;
        float const top_y = m_top_left.y + (m_top_right.y - m_top_left.y) * nx;
        ScreenPoint out;
        out.x = bottom_x + (top_x - bottom_x) * ny;
        out.y = bottom_y + (top_y - bottom_y) * ny;
        return out;
    }

private:
    ScreenPoint m_top_left { }, m_top_right { }, m_bottom_right { }, m_bottom_left { };
};

struct Vertex
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
};

struct BarVertex
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
    float u2, v2;
};

constexpr DWORD VERTEX_FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
constexpr DWORD BAR_FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2;
}