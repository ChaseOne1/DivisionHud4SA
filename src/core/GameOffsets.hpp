#pragma once

#include <cstdint>

namespace division::core {

// Holds the byte offset of the D3D9 texture handle inside the game's RwRaster
// extension block. WeaponPreview reads it to turn a native raster into an
// IDirect3DTexture9*.
inline constexpr std::uintptr_t RASTER_TEXTURE_OFFSET_ADDR = 0xB4E9E0;

// Address of the `call CEntity::RegisterReference` instruction the damage
// indicators replace so every same-frame damage source is observed.
inline constexpr std::uintptr_t REGISTER_REFERENCE_CALL_ADDR = 0x4B5C8D;

}