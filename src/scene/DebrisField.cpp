#include "DebrisField.h"

#include "core/HResult.h"

#include <cmath>
#include <cstring>
#include <numbers>
#include <stdexcept>
#include <algorithm>

namespace
{
    D3D12_RESOURCE_DESC CreateBufferDescription(
        std::uint64_t byteCount)
    {
        D3D12_RESOURCE_DESC description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_BUFFER;

        description.Alignment = 0;
        description.Width = byteCount;
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

        return description;
    }

    D3D12_HEAP_PROPERTIES CreateHeapProperties(
        D3D12_HEAP_TYPE heapType)
    {
        D3D12_HEAP_PROPERTIES properties{};

        properties.Type = heapType;

        properties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        properties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        properties.CreationNodeMask = 1;
        properties.VisibleNodeMask = 1;

        return properties;
    }

    float Hash01(std::uint32_t value) noexcept
    {
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;

        return
            static_cast<float>(
                value & 0x00ffffffu
                ) /
            static_cast<float>(
                0x01000000u
                );
    }

    DirectX::XMFLOAT4 QuaternionFromEuler(
        float pitch,
        float yaw,
        float roll) noexcept
    {
        DirectX::XMFLOAT4 result{};

        DirectX::XMStoreFloat4(
            &result,
            DirectX::XMQuaternionRotationRollPitchYaw(
                pitch,
                yaw,
                roll
            )
        );

        return result;
    }
}

namespace gf::scene
{
    DebrisField::DebrisField(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "DebrisField requires a valid device"
            );
        }

        if (commandList == nullptr)
        {
            throw std::invalid_argument(
                "DebrisField requires a valid command list"
            );
        }

        const std::vector<DebrisInstance>
            initialInstances =
            CreateInitialInstances();

        if (
            initialInstances.size() !=
            InstanceCount)
        {
            throw std::runtime_error(
                "Debris initialization count "
                "does not match InstanceCount"
            );
        }

        const UINT64 bufferSize =
            static_cast<UINT64>(
                initialInstances.size()
                ) *
            sizeof(DebrisInstance);

        const D3D12_RESOURCE_DESC
            bufferDescription =
            CreateBufferDescription(
                bufferSize
            );

        //
        // Create the GPU-local structured buffer.
        //
        // The CPU cannot map this buffer directly.
        // Rendering will read instances from here.
        //
        const D3D12_HEAP_PROPERTIES
            defaultHeapProperties =
            CreateHeapProperties(
                D3D12_HEAP_TYPE_DEFAULT
            );

        core::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDescription,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(
                    m_instanceBuffer
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris instance buffer"
        );

        core::ThrowIfFailed(
            m_instanceBuffer->SetName(
                L"Gravity Forge Debris Instances"
            ),
            "Failed to name debris instance buffer"
        );

        //
        // Create a temporary CPU-visible upload buffer.
        //
        const D3D12_HEAP_PROPERTIES
            uploadHeapProperties =
            CreateHeapProperties(
                D3D12_HEAP_TYPE_UPLOAD
            );

        core::ThrowIfFailed(
            device->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(
                    m_uploadBuffer
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris upload buffer"
        );

        core::ThrowIfFailed(
            m_uploadBuffer->SetName(
                L"Gravity Forge Debris Instance Upload"
            ),
            "Failed to name debris upload buffer"
        );

        //
        // Copy the CPU vector into the upload heap.
        //
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
            "Failed to map debris upload buffer"
        );

        std::memcpy(
            mappedMemory,
            initialInstances.data(),
            static_cast<std::size_t>(
                bufferSize
                )
        );

        const D3D12_RANGE cpuWrittenRange{
            0,
            static_cast<SIZE_T>(
                bufferSize
            )
        };

        m_uploadBuffer->Unmap(
            0,
            &cpuWrittenRange
        );

        //
        // Record the upload-buffer-to-default-buffer copy.
        //
        commandList->CopyBufferRegion(
            m_instanceBuffer.Get(),
            0,
            m_uploadBuffer.Get(),
            0,
            bufferSize
        );

        //
        // The copy destination becomes a vertex-shader SRV.
        //
        D3D12_RESOURCE_BARRIER
            instanceBufferBarrier{};

        instanceBufferBarrier.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        instanceBufferBarrier.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        instanceBufferBarrier
            .Transition
            .pResource =
            m_instanceBuffer.Get();

        instanceBufferBarrier
            .Transition
            .Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        instanceBufferBarrier
            .Transition
            .StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;

        instanceBufferBarrier
            .Transition
            .StateAfter =
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(
            1,
            &instanceBufferBarrier
        );
    }

    std::vector<DebrisInstance>
        DebrisField::CreateInitialInstances()
    {
        std::vector<DebrisInstance> result(
            InstanceCount
        );

        constexpr float twoPi =
            2.0f *
            std::numbers::pi_v<float>;

        // CreateBoxMesh() covers [-0.5, +0.5]
        // on all three axes.
        constexpr float boxLocalRadius =
            0.8660254f; // sqrt(3) / 2.

        for (
            std::uint32_t index = 0;
            index < InstanceCount;
            ++index)
        {
            const float radiusNoise =
                Hash01(index * 11u + 1u);

            const float phaseNoise =
                Hash01(index * 11u + 2u);

            const float heightNoise =
                Hash01(index * 11u + 3u);

            const float orbitSpeedNoise =
                Hash01(index * 11u + 4u);

            const float orbitTiltPitchNoise =
                Hash01(index * 11u + 5u);

            const float orbitTiltRollNoise =
                Hash01(index * 11u + 6u);

            const float scaleNoise =
                Hash01(index * 11u + 7u);

            const float orientationNoise =
                Hash01(index * 11u + 8u);

            const float tumbleAxisNoise =
                Hash01(index * 11u + 9u);

            const float tumbleSpeedNoise =
                Hash01(index * 11u + 10u);

            const float orbitDirection =
                (index & 1u) == 0u
                ? 1.0f
                : -1.0f;

            const float tumbleDirection =
                (index & 2u) == 0u
                ? 1.0f
                : -1.0f;

            DebrisInstance& instance =
                result[index];

            instance.orbit = {
                // Radius.
                2.0f +
                    1.25f * radiusNoise,

                // Initial phase.
                twoPi * phaseNoise,

                // Angular speed, radians/second.
                orbitDirection *
                    (
                        0.08f +
                        0.20f * orbitSpeedNoise
                    ),

                // Offset from the local orbit plane.
                -0.70f +
                    1.40f * heightNoise
            };

            instance.orbitOrientation =
                QuaternionFromEuler(
                    -0.28f +
                    0.56f *
                    orbitTiltPitchNoise,

                    0.0f,

                    -0.28f +
                    0.56f *
                    orbitTiltRollNoise
                );

            instance.localOrientation =
                QuaternionFromEuler(
                    twoPi *
                    orientationNoise,

                    twoPi *
                    Hash01(
                        index * 17u + 3u
                    ),

                    twoPi *
                    Hash01(
                        index * 17u + 5u
                    )
                );

            const float scaleX =
                0.10f +
                0.20f * scaleNoise;

            const float scaleY =
                0.035f +
                0.065f *
                Hash01(
                    index * 19u + 7u
                );

            const float scaleZ =
                0.030f +
                0.055f *
                Hash01(
                    index * 19u + 11u
                );

            const float maximumScale =
                std::max(
                    scaleX,
                    std::max(
                        scaleY,
                        scaleZ
                    )
                );

            instance.scaleAndBoundRadius = {
                scaleX,
                scaleY,
                scaleZ,

                // boxLocalRadius is sqrt(3) / 2.
                boxLocalRadius *
                    maximumScale
            };

            const float scaledCornerRadius =
                0.5f *
                std::sqrt(
                    scaleX * scaleX +
                    scaleY * scaleY +
                    scaleZ * scaleZ
                );

            if (
                scaledCornerRadius >
                instance.scaleAndBoundRadius.w +
                0.00001f)
            {
                throw std::runtime_error(
                    "Generated debris bound does not "
                    "enclose its mesh"
                );
            }

            const float tumbleAzimuth =
                twoPi * tumbleAxisNoise;

            const float tumbleY =
                -0.85f +
                1.70f *
                Hash01(
                    index * 23u + 13u
                );

            const float tumbleHorizontal =
                std::sqrt(
                    1.0f -
                    tumbleY * tumbleY
                );

            instance.tumble = {
                std::cos(tumbleAzimuth) *
                    tumbleHorizontal,

                tumbleY,

                std::sin(tumbleAzimuth) *
                    tumbleHorizontal,

                tumbleDirection *
                    (
                        0.35f +
                        1.65f *
                        tumbleSpeedNoise
                    )
            };
        }

        return result;
    }

    void DebrisField::ReleaseUploadResource()
        noexcept
    {
        m_uploadBuffer.Reset();
    }
}