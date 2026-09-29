#include "plugin.h"
#include "core/Config.hpp"
#include "hud/Panel3D.hpp"

using namespace plugin;

namespace {
division::hud::Panel3D g_panel;
}

class DivisionHud4SA
{
public:
    DivisionHud4SA()
    {
        division::core::Load();

        Events::drawingEvent += [] {
            g_panel.Update();
            g_panel.Draw();
        };
        Events::d3dLostEvent += [] { g_panel.OnDeviceLost(); };
    }
} division_hud_4sa;
