#include "plugin.h"
#include "Config.h"
#include "Panel3D.h"

using namespace plugin;

namespace
{
    division::Panel3D gPanel;
}

class DivisionHud4SA
{
public:
    DivisionHud4SA()
    {
        Config::Load();

        Events::drawingEvent += [] {
            gPanel.Update();
            gPanel.Draw();
        };
        Events::d3dLostEvent += [] { gPanel.OnDeviceLost(); };
    }
} divisionHud4SA;
