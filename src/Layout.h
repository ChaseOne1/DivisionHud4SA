#pragma once

namespace division {
namespace layout {
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

    inline PanelRects Compute(float columnLength, float skillHeight, float gap)
    {
        if (columnLength < 0.01f)
            columnLength = 0.01f;
        if (columnLength > 0.5f)
            columnLength = 0.5f;

        if (gap < 0.0f)
            gap = 0.0f;

        if (skillHeight < 0.01f)
            skillHeight = 0.01f;
        if (skillHeight > 0.5f)
            skillHeight = 0.5f;

        float rightWidth = 1.0f - columnLength - gap;
        if (rightWidth < 0.01f)
            rightWidth = 0.01f;

        PanelRects rects { };
        rects.ammo = { 0.0f, 0.0f, columnLength, 1.0f };

        float const rightX0 = columnLength + gap;
        rects.skill = { rightX0, 1.0f - skillHeight, rightX0 + rightWidth, 1.0f };
        rects.resource = { rightX0, 0.0f, rightX0 + rightWidth, skillHeight };
        return rects;
    }
}
}
