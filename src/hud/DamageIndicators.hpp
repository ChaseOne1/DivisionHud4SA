#pragma once

#include <d3d9.h>
#include <vector>

#include "render/Surface.hpp"

class CEntity;
class CPed;

namespace division::hud {

// Installs the call-site hook that delivers every damage event's attacker (call once at init).
void InstallDamageHook();

// Directional damage-source indicators drawn around the crosshair. Each source grows an arc as it
// keeps hitting, shrinks it once it stops, and disappears when it is forgotten or dies.
class DamageIndicators
{
public:
    void Update(CPed* ped, float dt);
    void Draw(IDirect3DDevice9* device, std::vector<render::BarVertex>& vertices, float alpha) const;

private:
    static constexpr int MAX_SOURCES = 32;

    struct Source
    {
        CEntity* entity = nullptr;
        bool pooled = false;
        unsigned char kind = 0;
        int ref = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float arc = 0.0f;
        float since = 0.0f;
        float angle = 0.0f;
    };

    Source m_sources[MAX_SOURCES] { };
    int m_count = 0;
    D3DCOLOR m_color = 0xFFFFFFFF;

    void RegisterHit(CEntity* attacker);
    CEntity* Resolve(Source const& source) const;
    void Remove(int index);
};

}
