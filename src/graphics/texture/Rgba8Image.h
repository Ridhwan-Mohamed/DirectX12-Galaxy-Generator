#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gf::graphics::texture
{
    struct Rgba8Image final
    {
        static constexpr std::size_t BytesPerPixel = 4;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::vector<std::uint8_t> pixels;

        [[nodiscard]]
        std::size_t SourceRowSizeInBytes() const noexcept
        {
            return static_cast<std::size_t>(width)
                * BytesPerPixel;
        }

        [[nodiscard]]
        std::uint8_t* Row(
            std::uint32_t rowIndex) noexcept
        {
            return pixels.data()
                + static_cast<std::size_t>(rowIndex)
                * SourceRowSizeInBytes();
        }

        [[nodiscard]]
        const std::uint8_t* Row(
            std::uint32_t rowIndex) const noexcept
        {
            return pixels.data()
                + static_cast<std::size_t>(rowIndex)
                * SourceRowSizeInBytes();
        }
    };

    [[nodiscard]]
    Rgba8Image CreateUvTestImage();
}