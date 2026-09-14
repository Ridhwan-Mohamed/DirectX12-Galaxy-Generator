#pragma once

#include "graphics/texture/Rgba8Image.h"

#include <filesystem>

namespace gf::graphics::texture
{
    [[nodiscard]]
    Rgba8Image LoadRgba8Image(
        const std::filesystem::path& filePath
    );
}