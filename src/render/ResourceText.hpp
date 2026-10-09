#pragma once

#include <string>

namespace division::render {

// Reads a text resource from the plugin's DivisionHud4SA resource folder (relative path),
// returning `fallback` if it cannot be read.
std::string LoadResourceText(char const* relative_path, char const* fallback);

}