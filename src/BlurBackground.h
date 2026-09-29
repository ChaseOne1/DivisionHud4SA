#pragma once

#include <d3d9.h>

namespace division {
class BlurBackground
{
public:
    ~BlurBackground();

    IDirect3DTexture9* Update(IDirect3DDevice9* device);
    void OnDeviceLost();

private:
    struct Target
    {
        IDirect3DTexture9* texture = nullptr;
        IDirect3DSurface9* surface = nullptr;
        int width = 0;
        int height = 0;
    };

    bool EnsureTargets(IDirect3DDevice9* device, int width, int height, D3DFORMAT format);
    bool EnsureShader(IDirect3DDevice9* device);
    void BlurPass(IDirect3DDevice9* device, Target& destination, IDirect3DTexture9* source, float stepX, float stepY);
    void ReleaseTarget(Target& target);
    void ReleaseTargets();

    Target m_half;
    Target m_quarter;
    Target m_temp;
    Target m_result;

    IDirect3DPixelShader9* m_pixelShader = nullptr;
    int m_screenWidth = 0;
    int m_screenHeight = 0;
};
}
