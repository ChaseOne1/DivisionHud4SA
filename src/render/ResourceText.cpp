#include "render/ResourceText.hpp"

#include "plugin.h"

namespace division::render {

std::string LoadResourceText(char const* relative_path, char const* fallback)
{
    HANDLE const file = CreateFileA(PLUGIN_PATH(relative_path), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size { };
        if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < (1 << 20)) {
            std::string text(static_cast<size_t>(size.QuadPart), '\0');
            DWORD read = 0;
            if (ReadFile(file, text.data(), static_cast<DWORD>(text.size()), &read, nullptr) && read > 0) {
                CloseHandle(file);
                text.resize(read);
                return text;
            }
        }
        CloseHandle(file);
    }
    return fallback ? std::string(fallback) : std::string();
}

}