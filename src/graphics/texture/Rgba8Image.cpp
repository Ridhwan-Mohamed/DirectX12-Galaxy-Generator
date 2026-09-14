#include "graphics/texture/Rgba8Image.h"

namespace gf::graphics::texture
{
    Rgba8Image CreateUvTestImage()
    {
        constexpr std::uint32_t width = 257;
        constexpr std::uint32_t height = 129;
        constexpr std::uint32_t tileSize = 16;

        Rgba8Image image;

        image.width = width;
        image.height = height;

        image.pixels.resize(
            static_cast<std::size_t>(width)
            * static_cast<std::size_t>(height)
            * Rgba8Image::BytesPerPixel
        );

        for (std::uint32_t y = 0; y < height; ++y)
        {
            for (std::uint32_t x = 0; x < width; ++x)
            {
                const bool lightTile =
                    ((x / tileSize) + (y / tileSize)) % 2 == 0;

                std::uint8_t red =
                    lightTile ? 220 : 35;

                std::uint8_t green =
                    lightTile ? 45 : 25;

                std::uint8_t blue =
                    lightTile ? 220 : 90;

                // Colored borders make orientation obvious.
                if (y < 4)
                {
                    red = 255;
                    green = 0;
                    blue = 0;
                }
                else if (y >= height - 4)
                {
                    red = 0;
                    green = 255;
                    blue = 255;
                }
                else if (x < 4)
                {
                    red = 0;
                    green = 255;
                    blue = 0;
                }
                else if (x >= width - 4)
                {
                    red = 255;
                    green = 255;
                    blue = 0;
                }

                // A center cross helps expose stretching.
                if (x == width / 2 || y == height / 2)
                {
                    red = 255;
                    green = 255;
                    blue = 255;
                }

                std::uint8_t* pixel =
                    image.Row(y)
                    + static_cast<std::size_t>(x)
                    * Rgba8Image::BytesPerPixel;

                pixel[0] = red;
                pixel[1] = green;
                pixel[2] = blue;
                pixel[3] = 255;
            }
        }

        return image;
    }
}