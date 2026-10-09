#pragma once

#include <d3d9.h>

namespace division::render {

IDirect3DPixelShader9* CompilePixelShader(IDirect3DDevice9* device, char const* source);
IDirect3DPixelShader9* CreateLineShader(IDirect3DDevice9* device);
IDirect3DPixelShader9* CreateBarShader(IDirect3DDevice9* device);
IDirect3DPixelShader9* CreateWaveShader(IDirect3DDevice9* device);

}