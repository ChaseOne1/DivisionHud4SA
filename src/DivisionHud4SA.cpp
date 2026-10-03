#include "plugin.h"
#include "core/Config.hpp"
#include "hud/DamageIndicators.hpp"
#include "hud/Panel3D.hpp"
#include "hud/WeaponPreview.hpp"
#include "render/BlurBackground.hpp"

using namespace plugin;

namespace {
division::hud::Panel3D g_panel;
division::hud::WeaponPreview g_weapon_preview;
division::render::BlurBackground g_blur;
}

class DivisionHud4SA
{
public:
    DivisionHud4SA()
    {
        division::core::Load();
        division::hud::InstallDamageHook();

        Events::drawingEvent += [] {
            g_panel.Update();
            g_weapon_preview.Update();

            // Blur the scene once per frame and share the result with both HUDs.
            IDirect3DTexture9* blurred = nullptr;
            IDirect3DDevice9* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
            if (device && FindPlayerPed(0) && !FindPlayerVehicle(0, false))
                blurred = g_blur.Update(device);

            g_panel.Draw(blurred);
            g_weapon_preview.Draw(blurred);
        };
        Events::d3dLostEvent += [] {
            g_panel.OnDeviceLost();
            g_weapon_preview.OnDeviceLost();
            g_blur.OnDeviceLost();
        };
    }
} division_hud_4sa;
