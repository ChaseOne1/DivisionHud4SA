#pragma once

#include <d3d9.h>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>

namespace division::render {
class TextRenderer
{
public:
    ~TextRenderer();

    void Initialize(char const* font_path, int pixel_height);
    void Shutdown();
    void OnDeviceLost();

    // Lookup is heterogeneous: char buffers, string literals and std::string all bind to
    // std::string_view, so re-requesting a cached glyph never allocates or copies a key.
    IDirect3DTexture9* Get(IDirect3DDevice9* device, std::string_view text, int& width, int& height);

private:
    // Transparent hash/equality let find() run on std::string_view without building a key.
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(std::string const& key) const noexcept
        {
            return std::hash<std::string_view> {}(key);
        }
        size_t operator()(std::string_view key) const noexcept
        {
            return std::hash<std::string_view> {}(key);
        }
    };

    struct Entry
    {
        IDirect3DTexture9* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    IDirect3DTexture9* CreateTexture(IDirect3DDevice9* device, std::string_view text, int& width, int& height);
    void EvictOldest();

    HFONT m_font = nullptr;
    std::string m_font_path;
    // The map owns the glyphs; the list records insertion order so a full cache can drop
    // one old entry per new request instead of wiping every texture in a single frame.
    std::unordered_map<std::string, Entry, StringHash, std::equal_to<>> m_cache;
    std::list<std::string> m_insertion_order;
};
}