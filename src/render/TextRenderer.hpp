#pragma once

#include <d3d9.h>
#include <map>
#include <string>

namespace division::render {
class TextRenderer
{
public:
    ~TextRenderer();

    void Initialize(char const* font_path, int pixel_height);
    void Shutdown();
    void OnDeviceLost();

    IDirect3DTexture9* Get(IDirect3DDevice9* device, std::string const& text, int& width, int& height);

private:
    struct Entry
    {
        IDirect3DTexture9* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    IDirect3DTexture9* CreateTexture(IDirect3DDevice9* device, std::string const& text, int& width, int& height);

    HFONT m_font = nullptr;
    std::string m_font_path;
    std::map<std::string, Entry> m_cache;
};
}
