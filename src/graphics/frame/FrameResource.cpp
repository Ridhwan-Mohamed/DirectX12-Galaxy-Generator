#include "FrameResource.h"

#include <stdexcept>
#include <utility>
#include "../../core/HResult.h"
#include <cstring>
#include <format>
#include <cassert>


namespace
{
    constexpr std::uint64_t
        SceneConstantBufferSize =
        gf::graphics::frame::FrameResource::
        SceneConstantBufferStride *
        gf::graphics::frame::FrameResource::
        MaxSceneObjects;

    static_assert(
        sizeof(
            gf::graphics::frame::
            SceneConstants
            ) <=
        gf::graphics::frame::FrameResource::
        SceneConstantBufferStride
        );

    static_assert(
        SceneConstantBufferSize == 8192
        );
}

namespace gf::graphics::frame
{
    FrameResource::FrameResource(
        ID3D12Device* device,
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator,
        std::uint32_t frameIndex
    ) : m_commandAllocator(std::move(commandAllocator))
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "FrameResource requires a device"
            );
        }

        if (m_commandAllocator == nullptr)
        {
            throw std::invalid_argument(
                "FrameResource requires "
                "a command allocator"
            );
        }

        D3D12_HEAP_PROPERTIES
            heapProperties{};

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

        description.Width =
            SceneConstantBufferSize;

        description.Height = 1;
        description.DepthOrArraySize = 1;
        description.MipLevels = 1;

        description.Format =
            DXGI_FORMAT_UNKNOWN;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        description.Flags =
            D3D12_RESOURCE_FLAG_NONE;

        core::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(
                    m_sceneConstantBuffer.ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create frame "
            "constant buffer"
        );

        const std::wstring bufferName =
            std::format(
                L"Gravity Forge Frame {} "
                L"Scene Constants",
                frameIndex
            );

        core::ThrowIfFailed(
            m_sceneConstantBuffer->SetName(
                bufferName.c_str()
            ),
            "Failed to name frame "
            "constant buffer"
        );

        void* mappedMemory = nullptr;

        const D3D12_RANGE cpuReadRange{
            0,
            0
        };

        core::ThrowIfFailed(
            m_sceneConstantBuffer->Map(
                0,
                &cpuReadRange,
                &mappedMemory
            ),
            "Failed to map frame "
            "constant buffer"
        );

        m_mappedSceneConstants =
            static_cast<std::byte*>(
                mappedMemory
                );
    }

    FrameResource::~FrameResource()
    {
        if (m_sceneConstantBuffer != nullptr &&
            m_mappedSceneConstants != nullptr)
        {
            m_sceneConstantBuffer->Unmap(
                0,
                nullptr
            );

            m_mappedSceneConstants = nullptr;
        }
    }

    FrameResource::FrameResource(
        FrameResource&& other) noexcept
        : m_commandAllocator(
            std::move(
                other.m_commandAllocator
            )
        ),
        m_sceneConstantBuffer(
            std::move(
                other.m_sceneConstantBuffer
            )
        ),
        m_mappedSceneConstants(
            std::exchange(
                other.m_mappedSceneConstants,
                nullptr
            )
        ),
        m_fenceValue(
            std::exchange(
                other.m_fenceValue,
                0
            )
        )
    {
    }

    D3D12_GPU_VIRTUAL_ADDRESS
        FrameResource::
        SceneConstantBufferAddress(
            std::uint32_t objectIndex
        ) const noexcept
    {
        assert(
            objectIndex < MaxSceneObjects
        );

        return m_sceneConstantBuffer
            ->GetGPUVirtualAddress() +
            objectIndex *
            SceneConstantBufferStride;
    }

    void FrameResource::WriteSceneConstants(
        std::uint32_t objectIndex,
        const SceneConstants& constants) noexcept
    {
        assert(
            objectIndex < MaxSceneObjects
        );

        const std::size_t byteOffset =
            static_cast<std::size_t>(
                objectIndex *
                SceneConstantBufferStride
                );

        std::memcpy(
            m_mappedSceneConstants +
            byteOffset,
            &constants,
            sizeof(constants)
        );
    }

    void FrameResource::
        WriteParticleRenderConstants(
            std::uint32_t objectIndex,
            const ParticleRenderConstants& constants
        ) noexcept
    {
        assert(
            objectIndex < MaxSceneObjects
        );

        const std::size_t byteOffset =
            static_cast<std::size_t>(
                objectIndex *
                SceneConstantBufferStride
                );

        std::memcpy(
            m_mappedSceneConstants +
            byteOffset,
            &constants,
            sizeof(constants)
        );
    }
}