#include "graphics/texture/WicImageLoader.h"

#include "core/HResult.h"

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <limits>
#include <stdexcept>

namespace gf::graphics::texture
{
    Rgba8Image LoadRgba8Image(
        const std::filesystem::path& filePath)
    {
        if (!std::filesystem::exists(filePath))
        {
            throw std::runtime_error(
                "Texture image file does not exist"
            );
        }

        Microsoft::WRL::ComPtr<IWICImagingFactory>
            factory;

        core::ThrowIfFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(
                    factory.ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create WIC imaging factory"
        );

        Microsoft::WRL::ComPtr<IWICBitmapDecoder>
            decoder;

        core::ThrowIfFailed(
            factory->CreateDecoderFromFilename(
                filePath.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.ReleaseAndGetAddressOf()
            ),
            "Failed to create WIC image decoder"
        );

        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode>
            frame;

        core::ThrowIfFailed(
            decoder->GetFrame(
                0,
                frame.ReleaseAndGetAddressOf()
            ),
            "Failed to get WIC image frame"
        );

        UINT width = 0;
        UINT height = 0;

        core::ThrowIfFailed(
            frame->GetSize(
                &width,
                &height
            ),
            "Failed to get decoded image size"
        );

        if (width == 0 || height == 0)
        {
            throw std::runtime_error(
                "Decoded image has invalid dimensions"
            );
        }

        WICPixelFormatGUID sourceFormat{};

        core::ThrowIfFailed(
            frame->GetPixelFormat(
                &sourceFormat
            ),
            "Failed to query source pixel format"
        );

        Microsoft::WRL::ComPtr<IWICFormatConverter>
            converter;

        core::ThrowIfFailed(
            factory->CreateFormatConverter(
                converter.ReleaseAndGetAddressOf()
            ),
            "Failed to create WIC format converter"
        );

        BOOL canConvert = FALSE;

        core::ThrowIfFailed(
            converter->CanConvert(
                sourceFormat,
                GUID_WICPixelFormat32bppRGBA,
                &canConvert
            ),
            "Failed to query WIC pixel conversion"
        );

        if (!canConvert)
        {
            throw std::runtime_error(
                "WIC cannot convert image to RGBA8"
            );
        }

        core::ThrowIfFailed(
            converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom
            ),
            "Failed to initialize WIC RGBA converter"
        );

        Rgba8Image image;

        image.width = width;
        image.height = height;

        const std::size_t rowSize =
            image.SourceRowSizeInBytes();

        if (height >
            std::numeric_limits<std::size_t>::max()
            / rowSize)
        {
            throw std::length_error(
                "Decoded image size overflow"
            );
        }

        const std::size_t imageSize =
            rowSize
            * static_cast<std::size_t>(height);

        if (rowSize >
            std::numeric_limits<UINT>::max() ||
            imageSize >
            std::numeric_limits<UINT>::max())
        {
            throw std::length_error(
                "Decoded image exceeds WIC copy limits"
            );
        }

        image.pixels.resize(imageSize);

        core::ThrowIfFailed(
            converter->CopyPixels(
                nullptr,
                static_cast<UINT>(rowSize),
                static_cast<UINT>(imageSize),
                image.pixels.data()
            ),
            "Failed to copy decoded RGBA pixels"
        );

        return image;
    }
}