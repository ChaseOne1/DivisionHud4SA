#include "render/TextRenderer.hpp"

#include <cstring>

namespace division::render {
namespace {
    struct FaceSearch
    {
        std::string name;
    };

    int CALLBACK FindFaceProc(LOGFONTA const* log_font, TEXTMETRICA const*, DWORD, LPARAM param)
    {
        if (log_font->lfFaceName[0] == '@')
            return 1;

        FaceSearch* search = reinterpret_cast<FaceSearch*>(param);
        std::string const name = log_font->lfFaceName;
        if (name.find("Borda") != std::string::npos) {
            search->name = name;
            return 0;
        }
        return 1;
    }
}

TextRenderer::~TextRenderer()
{
    Shutdown();
}

void TextRenderer::Initialize(char const* font_path, int pixel_height)
{
    if (m_font)
        return;

    m_font_path = font_path ? font_path : "";
    if (!m_font_path.empty())
        AddFontResourceExA(m_font_path.c_str(), FR_PRIVATE, nullptr);

    std::string face_name = "Borda Medium";
    HDC screen = GetDC(nullptr);
    if (screen) {
        FaceSearch search;
        LOGFONTA log_font { };
        log_font.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExA(screen, &log_font, FindFaceProc, reinterpret_cast<LPARAM>(&search), 0);
        if (!search.name.empty())
            face_name = search.name;
        ReleaseDC(nullptr, screen);
    }

    m_font = CreateFontA(-pixel_height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, face_name.c_str());
}

void TextRenderer::Shutdown()
{
    for (auto& entry : m_cache) {
        if (entry.second.texture)
            entry.second.texture->Release();
    }
    m_cache.clear();

    if (m_font) {
        DeleteObject(m_font);
        m_font = nullptr;
    }

    if (!m_font_path.empty()) {
        RemoveFontResourceExA(m_font_path.c_str(), FR_PRIVATE, nullptr);
        m_font_path.clear();
    }
}

void TextRenderer::OnDeviceLost()
{
    for (auto& entry : m_cache) {
        if (entry.second.texture)
            entry.second.texture->Release();
    }
    m_cache.clear();
}

IDirect3DTexture9* TextRenderer::Get(IDirect3DDevice9* device, std::string const& text, int& width, int& height)
{
    auto it = m_cache.find(text);
    if (it != m_cache.end()) {
        width = it->second.width;
        height = it->second.height;
        return it->second.texture;
    }

    constexpr size_t MAX_CACHE = 256;
    if (m_cache.size() >= MAX_CACHE) {
        for (auto& entry : m_cache) {
            if (entry.second.texture)
                entry.second.texture->Release();
        }
        m_cache.clear();
    }

    Entry entry;
    entry.texture = CreateTexture(device, text, entry.width, entry.height);
    width = entry.width;
    height = entry.height;
    if (!entry.texture)
        return nullptr;

    m_cache[text] = entry;
    return entry.texture;
}

IDirect3DTexture9* TextRenderer::CreateTexture(IDirect3DDevice9* device, std::string const& text, int& width,
    int& height)
{
    width = 0;
    height = 0;
    if (!m_font || text.empty())
        return nullptr;

    HDC screen = GetDC(nullptr);
    if (!screen)
        return nullptr;

    HDC dc = CreateCompatibleDC(screen);
    if (!dc) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }

    HGDIOBJ const old_font = SelectObject(dc, m_font);

    SIZE extent { };
    GetTextExtentPoint32A(dc, text.c_str(), static_cast<int>(text.size()), &extent);
    width = (extent.cx > 0) ? extent.cx : 1;
    height = (extent.cy > 0) ? extent.cy : 1;

    BITMAPINFO info { };
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP const dib = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        SelectObject(dc, old_font);
        DeleteDC(dc);
        ReleaseDC(nullptr, screen);
        return nullptr;
    }

    HGDIOBJ const old_bitmap = SelectObject(dc, dib);
    std::memset(bits, 0, static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    SetTextAlign(dc, TA_TOP | TA_LEFT);
    TextOutA(dc, 0, 0, text.c_str(), static_cast<int>(text.size()));

    IDirect3DTexture9* texture = nullptr;
    if (SUCCEEDED(device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr))) {
        D3DLOCKED_RECT locked { };
        if (SUCCEEDED(texture->LockRect(0, &locked, nullptr, 0))) {
            unsigned int const* source = static_cast<unsigned int const*>(bits);
            for (int y = 0; y < height; ++y) {
                unsigned int* dest = reinterpret_cast<unsigned int*>(
                    static_cast<unsigned char*>(locked.pBits) + static_cast<size_t>(y) * locked.Pitch);
                for (int x = 0; x < width; ++x) {
                    unsigned int const coverage = source[static_cast<size_t>(y) * width + x] & 0xffu;
                    dest[x] = (coverage << 24) | 0x00ffffffu;
                }
            }
            texture->UnlockRect(0);
        }
    }

    SelectObject(dc, old_bitmap);
    DeleteObject(dib);
    SelectObject(dc, old_font);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    return texture;
}
}
