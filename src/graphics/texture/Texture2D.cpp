#include "graphics/texture/Texture2D.h"

#include "core/HResult.h"
#include "graphics/texture/Rgba8Image.h"

#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace
{
    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateUploadBuffer(
            ID3D12Device* device,
            std::uint64_t byteCount)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};

        heapProperties.Type =
            D3D12_HEAP_TYPE_UPLOAD;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_BUFFER;

        description.Alignment = 0;
        description.Width = byteCount;
        description.Height = 1;
        description.DepthOrArraySize = 1;
        description.MipLevels = 1;
        description.Format = DXGI_FORMAT_UNKNOWN;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        description.Flags =
            D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            uploadBuffer;

        gf::core::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(
                    uploadBuffer.ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create texture upload buffer"
        );

        return uploadBuffer;
    }
}

namespace gf::graphics::texture
{
    Texture2D::Texture2D(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const Rgba8Image& image,
        std::wstring_view debugName)
        : m_width(image.width),
        m_height(image.height)
    {
        if (debugName.empty())
        {
            throw std::invalid_argument(
                "Texture2D requires a debug name"
            );
        }

        if (device == nullptr ||
            commandList == nullptr)
        {
            throw std::invalid_argument(
                "Texture2D requires a device "
                "and command list"
            );
        }

        if (image.width == 0 ||
            image.height == 0)
        {
            throw std::invalid_argument(
                "Texture2D requires a nonempty image"
            );
        }

        const std::size_t sourceRowSize =
            image.SourceRowSizeInBytes();

        if (image.height >
            std::numeric_limits<std::size_t>::max()
            / sourceRowSize)
        {
            throw std::length_error(
                "Texture image size overflow"
            );
        }

        const std::size_t expectedImageSize =
            sourceRowSize
            * static_cast<std::size_t>(image.height);

        if (image.pixels.size() != expectedImageSize)
        {
            throw std::invalid_argument(
                "Texture pixel data size "
                "does not match its dimensions"
            );
        }

        D3D12_RESOURCE_DESC textureDescription{};

        textureDescription.Dimension =
            D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        textureDescription.Alignment = 0;
        textureDescription.Width = image.width;
        textureDescription.Height = image.height;
        textureDescription.DepthOrArraySize = 1;
        textureDescription.MipLevels = 1;

        textureDescription.Format = m_format;

        textureDescription.SampleDesc.Count = 1;
        textureDescription.SampleDesc.Quality = 0;

        textureDescription.Layout =
            D3D12_TEXTURE_LAYOUT_UNKNOWN;

        textureDescription.Flags =
            D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES defaultHeapProperties{};

        defaultHeapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        defaultHeapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        defaultHeapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        defaultHeapProperties.CreationNodeMask = 1;
        defaultHeapProperties.VisibleNodeMask = 1;

        core::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDescription,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(
                    m_texture.ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create default-heap texture"
        );

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT
            placedFootprint{};

        UINT rowCount = 0;
        UINT64 unpaddedRowSize = 0;
        UINT64 uploadByteCount = 0;

        device->GetCopyableFootprints(
            &textureDescription,
            0,
            1,
            0,
            &placedFootprint,
            &rowCount,
            &unpaddedRowSize,
            &uploadByteCount
        );

        if (rowCount != image.height)
        {
            throw std::runtime_error(
                "Unexpected texture footprint row count"
            );
        }

        if (unpaddedRowSize != sourceRowSize)
        {
            throw std::runtime_error(
                "Texture footprint row size "
                "does not match source image"
            );
        }

        if (uploadByteCount >
            std::numeric_limits<std::size_t>::max())
        {
            throw std::length_error(
                "Texture upload is too large "
                "for CPU address space"
            );
        }

        m_uploadBuffer = CreateUploadBuffer(
            device,
            uploadByteCount
        );

        void* mappedMemory = nullptr;

        const D3D12_RANGE cpuReadRange{
            0,
            0
        };

        core::ThrowIfFailed(
            m_uploadBuffer->Map(
                0,
                &cpuReadRange,
                &mappedMemory
            ),
            "Failed to map texture upload buffer"
        );

        auto* uploadBytes =
            static_cast<std::uint8_t*>(
                mappedMemory
                );

        const std::size_t destinationRowPitch =
            placedFootprint.Footprint.RowPitch;

        const std::size_t destinationOffset =
            static_cast<std::size_t>(
                placedFootprint.Offset
                );

        for (UINT row = 0; row < rowCount; ++row)
        {
            std::uint8_t* destinationRow =
                uploadBytes
                + destinationOffset
                + static_cast<std::size_t>(row)
                * destinationRowPitch;

            const std::uint8_t* sourceRow =
                image.Row(row);

            std::memcpy(
                destinationRow,
                sourceRow,
                sourceRowSize
            );
        }

        const D3D12_RANGE cpuWrittenRange{
            0,
            static_cast<SIZE_T>(uploadByteCount)
        };

        m_uploadBuffer->Unmap(
            0,
            &cpuWrittenRange
        );

        D3D12_TEXTURE_COPY_LOCATION
            destination{};

        destination.pResource =
            m_texture.Get();

        destination.Type =
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        destination.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION
            source{};

        source.pResource =
            m_uploadBuffer.Get();

        source.Type =
            D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

        source.PlacedFootprint =
            placedFootprint;

        commandList->CopyTextureRegion(
            &destination,
            0,
            0,
            0,
            &source,
            nullptr
        );

        D3D12_RESOURCE_BARRIER barrier{};

        barrier.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        barrier.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        barrier.Transition.pResource =
            m_texture.Get();

        barrier.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barrier.Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;

        barrier.Transition.StateAfter =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(
            1,
            &barrier
        );

        const std::wstring textureName{
            debugName
        };

        const std::wstring uploadName =
            textureName + L" Upload";

        core::ThrowIfFailed(
            m_texture->SetName(
                textureName.c_str()
            ),
            "Failed to name texture"
        );

        core::ThrowIfFailed(
            m_uploadBuffer->SetName(
                uploadName.c_str()
            ),
            "Failed to name texture upload buffer"
        );
    }

    void Texture2D::ReleaseUploadResource() noexcept
    {
        m_uploadBuffer.Reset();
    }
}