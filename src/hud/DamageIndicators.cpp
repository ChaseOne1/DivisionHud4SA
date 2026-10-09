#include "hud/DamageIndicators.hpp"

#include "plugin.h"

#include "common.h"
#include "CCamera.h"
#include "CEntity.h"
#include "CPed.h"
#include "CPools.h"
#include "CVehicle.h"
#include "eEntityType.h"

#include <algorithm>
#include <cmath>

#include "core/Config.hpp"
#include "core/Math.hpp"
#include "core/GameOffsets.hpp"
#include "render/Primitives.hpp"
#include "render/Surface.hpp"

namespace division::hud {
using namespace render;

namespace {
    constexpr int MAX_HITS = 64;

    // Hits queued by the hook during the game phase, drained by Update in the draw phase.
    CEntity* g_hits[MAX_HITS];
    int g_hit_count = 0;

    // CEntity::RegisterReference(CEntity**): thiscall, this = referenced entity, arg = slot address.
    using RegisterReferenceFn = void(__thiscall*)(CEntity*, CEntity**);
    RegisterReferenceFn g_register_reference = nullptr;

    void __fastcall HookRegisterReference(CEntity* self, void* /*edx*/, CEntity** dest)
    {
        // The game stores only the last attacker, so this hook is the only way to see every
        // same-frame source. `dest` is the victim's m_pLastEntityDamage slot.
        CPed* const player = FindPlayerPed(0);
        if (player && self && self != player && dest == &player->m_pLastEntityDamage && g_hit_count < MAX_HITS)
            g_hits[g_hit_count++] = self;

        if (g_register_reference)
            g_register_reference(self, dest);
    }
}

void InstallDamageHook()
{
    if (g_register_reference)
        return;
    g_register_reference = reinterpret_cast<RegisterReferenceFn>(
        injector::MakeCALL(core::REGISTER_REFERENCE_CALL_ADDR, &HookRegisterReference).get<void>());
}

CEntity* DamageIndicators::Resolve(Source const& source) const
{
    if (!source.pooled) return nullptr;
    if (source.kind == ENTITY_TYPE_PED) return CPools::ms_pPedPool->GetAtRef(source.ref);
    if (source.kind == ENTITY_TYPE_VEHICLE) return CPools::ms_pVehiclePool->GetAtRef(source.ref);
    return nullptr;
}

void DamageIndicators::Remove(int index)
{
    m_sources[index] = m_sources[--m_count];
}

void DamageIndicators::RegisterHit(CEntity* attacker)
{
    if (!attacker) return;

    int index = -1;
    for (int i = 0; i < m_count; ++i) {
        if (m_sources[i].entity == attacker) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        int const cap = std::clamp(core::cfg.PanelDamageMaxSources, 1, MAX_SOURCES);
        if (m_count < cap) {
            index = m_count++;
        } else {
            index = 0;
            for (int i = 1; i < m_count; ++i)
                if (m_sources[i].since > m_sources[index].since) index = i;
        }

        m_sources[index] = Source { };
        m_sources[index].entity = attacker;
        if (attacker->m_nType == ENTITY_TYPE_PED) {
            m_sources[index].pooled = true;
            m_sources[index].kind = ENTITY_TYPE_PED;
            m_sources[index].ref = CPools::ms_pPedPool->GetRef(static_cast<CPed*>(attacker));
        } else if (attacker->m_nType == ENTITY_TYPE_VEHICLE) {
            m_sources[index].pooled = true;
            m_sources[index].kind = ENTITY_TYPE_VEHICLE;
            m_sources[index].ref = CPools::ms_pVehiclePool->GetRef(static_cast<CVehicle*>(attacker));
        }
        CVector const pos = attacker->GetPosition();
        m_sources[index].x = pos.x;
        m_sources[index].y = pos.y;
        m_sources[index].z = pos.z;
    }

    float const max_arc = core::cfg.PanelDamageArcMaxDeg * core::DEG_TO_RAD;
    float const min_arc = std::min(core::cfg.PanelDamageArcMinDeg * core::DEG_TO_RAD, max_arc);
    m_sources[index].arc = std::clamp(m_sources[index].arc + core::cfg.PanelDamageArcGainDeg * core::DEG_TO_RAD, min_arc, max_arc);
    m_sources[index].since = 0.0f;
}

void DamageIndicators::Update(CPed* ped, float dt)
{
    for (int i = 0; i < g_hit_count; ++i)
        RegisterHit(g_hits[i]);
    g_hit_count = 0;

    m_color = (ped && ped->m_fArmour > 0.0f) ? core::cfg.PanelDamageColorArmor : core::cfg.PanelDamageColorHealth;

    CVector forward = TheCamera.m_mCameraMatrix.GetForward();
    // SA's camera matrix stores its "right" as up x forward, which points to the camera's left;
    // negate so +right is screen-right (ahead = up, right of screen = +90 degrees).
    CVector right = TheCamera.m_mCameraMatrix.GetRight() * -1.0f;
    forward.z = 0.0f;
    right.z = 0.0f;
    if (forward.MagnitudeSqr() < 1e-6f) forward = CVector(0.0f, 1.0f, 0.0f);
    if (right.MagnitudeSqr() < 1e-6f) right = CVector(1.0f, 0.0f, 0.0f);
    forward.Normalize();
    right.Normalize();
    CVector const camera = TheCamera.m_mCameraMatrix.GetPosition();

    for (int i = 0; i < m_count; ) {
        Source& source = m_sources[i];

        if (source.pooled) {
            CEntity* const entity = Resolve(source);
            if (!entity) {
                Remove(i);
                continue;
            }
            if (source.kind == ENTITY_TYPE_PED) {
                CPed* const other = static_cast<CPed*>(entity);
                if (other->m_fHealth <= 0.0f || other->m_ePedState == PEDSTATE_DEAD
                    || other->m_ePedState == PEDSTATE_DIE || other->m_ePedState == PEDSTATE_DIE_BY_STEALTH) {
                    Remove(i);
                    continue;
                }
            }
            CVector const pos = entity->GetPosition();
            source.x = pos.x;
            source.y = pos.y;
            source.z = pos.z;
        }

        source.since += dt;
        if (source.since > core::cfg.PanelDamageArcHold)
            source.arc = std::max(0.0f, source.arc - core::cfg.PanelDamageArcShrinkDeg * core::DEG_TO_RAD * dt);
        if (source.since > core::cfg.PanelDamageForget) {
            Remove(i);
            continue;
        }

        float const dx = source.x - camera.x;
        float const dy = source.y - camera.y;
        float const mag2 = dx * dx + dy * dy;
        if (mag2 > 0.0001f) {
            float const inv = 1.0f / std::sqrt(mag2);
            float const nx = dx * inv;
            float const ny = dy * inv;
            source.angle = std::atan2(nx * right.x + ny * right.y, nx * forward.x + ny * forward.y);
        }

        ++i;
    }
}

void DamageIndicators::Draw(IDirect3DDevice9* device, std::vector<BarVertex>& vertices, float alpha) const
{
    if (m_count <= 0) return;

    float const width = static_cast<float>(RsGlobal.maximumWidth);
    float const height = static_cast<float>(RsGlobal.maximumHeight);
    float const cx = width * core::cfg.PanelDamageCenterX;
    float const cy = height * core::cfg.PanelDamageCenterY;
    D3DCOLOR const color = ScaleColor(m_color, alpha);

    vertices.clear();
    for (int i = 0; i < m_count; ++i) {
        Source const& source = m_sources[i];
        // Pointer first, then arc, so the arc band covers the pointer's tail.
        AppendPointerScreen(vertices, cx, cy, core::cfg.PanelDamageRadius, source.angle, core::cfg.PanelDamagePointerSize,
            core::cfg.PanelDamagePointerWidth, color);
        if (source.arc > 0.0f)
            AppendArcScreen(vertices, cx, cy, core::cfg.PanelDamageRadius, core::cfg.PanelDamageArcThickness, source.angle,
                source.arc * 0.5f, color);
    }

    if (!vertices.empty())
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(),
            sizeof(BarVertex));
}

}
