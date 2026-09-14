#include "ParticleSystem.h"

#include "../core/HResult.h"

#include <cmath>
#include <cstring>
#include <numbers>
#include <stdexcept>
#include <algorithm>

namespace gf::simulation
{

    namespace
    {
        [[nodiscard]]
        std::uint32_t HashParticleValue(
            std::uint32_t value
        ) noexcept
        {
            value ^= value >> 16;
            value *= 0x7feb352du;

            value ^= value >> 15;
            value *= 0x846ca68bu;

            value ^= value >> 16;

            return value;
        }


        [[nodiscard]]
        float HashParticle01(
            std::uint32_t value
        ) noexcept
        {
            constexpr float inverse24BitRange =
                1.0f /
                16777216.0f;

            return
                static_cast<float>(
                    HashParticleValue(value) >>
                    8
                    ) *
                inverse24BitRange;
        }
    }

    ParticleSystem::ParticleSystem(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const GalaxyParameters& galaxyParameters)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "ParticleSystem requires "
                "a valid D3D12 device"
            );
        }

        if (commandList == nullptr)
        {
            throw std::invalid_argument(
                "ParticleSystem requires "
                "a valid command list"
            );
        }

        const std::vector<ParticleState>
            initialStates =
            CreateInitialStates(
                galaxyParameters
            );

        CreateStateBuffers(
            device,
            commandList,
            initialStates
        );
    }

    std::vector<ParticleState> ParticleSystem::CreateInitialStates(const GalaxyParameters& galaxyParameters)
    {
        std::vector<ParticleState>
            particles;

        particles.resize(
            ParticleCount
        );

        constexpr float twoPi =
            2.0f *
            std::numbers::pi_v<float>;

        constexpr float goldenAngle =
            2.39996322972865332f;

        const float coreParticleFraction =
            std::clamp(
                galaxyParameters.coreParticleFraction,
                0.0f,
                1.0f
            );


        const std::uint32_t safeArmCount =
            std::max(
                galaxyParameters.armCount,
                1u
            );


        const float innerArmRadius =
            std::max(
                galaxyParameters.innerArmRadius,
                0.0001f
            );


        const float outerArmRadius =
            std::max(
                galaxyParameters.outerArmRadius,
                innerArmRadius
            );

        const float safeArmRadialSpan =
            std::max(
                outerArmRadius - innerArmRadius,
                0.0001f
            );

        const float regularArmHalfWidthInnerRadians =
            galaxyParameters.regularArmHalfWidthInnerDegrees *
            std::numbers::pi_v<float> /
            180.0f;

        const float interArmFractionInner =
            std::clamp(
                galaxyParameters.interArmFractionInner,
                0.0f,
                1.0f
            );

        const float interArmFractionOuter =
            std::clamp(
                galaxyParameters.interArmFractionOuter,
                0.0f,
                1.0f
            );

        const float interArmFractionRadiusPower =
            std::max(
                galaxyParameters
                .interArmFractionRadiusPower,
                0.0001f
            );

        const float interArmOuterBias =
            std::max(
                galaxyParameters.interArmFractionOuterBias,
                0.0f
            );

        for (
            std::uint32_t index = 0;
            index < ParticleCount;
            ++index)
        {
            const float particleIndex =
                static_cast<float>(index);

            const float populationSample =
                std::fmod(
                    particleIndex *
                    0.754877666f,
                    1.0f
                );

            const bool isCoreParticle =
                populationSample <
                coreParticleFraction;

            if (isCoreParticle)
            {
                const float radialSample =
                    std::fmod(
                        particleIndex *
                        0.569840291f +
                        0.417f,
                        1.0f
                    );

                const float safeCoreFalloff =
                    std::max(
                        galaxyParameters.coreFalloff,
                        0.001f
                    );

                const float radius =
                    galaxyParameters.coreRadius *
                    std::pow(
                        radialSample,
                        safeCoreFalloff
                    );

                const float angle =
                    std::fmod(
                        particleIndex *
                        goldenAngle,
                        twoPi
                    );

                const float cosAngle =
                    std::cos(angle);

                const float sinAngle =
                    std::sin(angle);

                const float heightSample =
                    std::fmod(
                        particleIndex *
                        0.438579021f +
                        0.271f,
                        1.0f
                    );

                const float normalizedRadius =
                    galaxyParameters.coreRadius >
                    0.0001f
                    ? radius /
                    galaxyParameters.coreRadius
                    : 0.0f;

                const float heightEnvelope =
                    std::sqrt(
                        std::max(
                            0.0f,
                            1.0f -
                            normalizedRadius *
                            normalizedRadius
                        )
                    );

                const float height =
                    (
                        heightSample *
                        2.0f -
                        1.0f
                        ) *
                    galaxyParameters
                    .coreHalfThickness *
                    heightEnvelope;

                ParticleState& particle =
                    particles[index];

                particle.position =
                    DirectX::XMFLOAT3{
                        cosAngle * radius,
                        height,
                        sinAngle * radius
                };

                //
                // Initialize tangential velocity from the
                // same differential rotation curve used by
                // the GPU simulation:
                //
                //      v = r * omega(r)
                //
                const float angularVelocity =
                    ComputeGalaxyAngularVelocityAtRadius(
                        galaxyParameters,
                        radius
                    );


                const float coreOrbitSpeed =
                    radius *
                    angularVelocity;

                particle.velocity =
                    DirectX::XMFLOAT3{
                        -sinAngle *
                            coreOrbitSpeed,
                        0.0f,
                        cosAngle *
                            coreOrbitSpeed
                };

                particle.age = 0.0f;

                particle.seed =
                    std::fmod(
                        particleIndex *
                        0.61803398875f,
                        1.0f
                    );

                continue;
            }

            //
            // Assign every non-core particle to one
            // of the coherent spiral arms.
            //
            // Using particle index modulo arm count
            // makes arm identity deterministic and
            // keeps prefix draw-count modes balanced.
            //
            const std::uint32_t armIndex =
                index %
                safeArmCount;


            //
            // Deterministic low-discrepancy radial
            // sample in [0, 1).
            //
            const float radialSample =
                HashParticle01(
                    index * 6u + 0u
                );


            //
            // Uniformly populate the arm from the
            // inner radius to the outer radius.
            //
            const float baseRadius =
                innerArmRadius +
                (
                    outerArmRadius -
                    innerArmRadius
                    ) *
                radialSample;

            const float normalizedRadius =
                std::clamp(
                    (baseRadius - innerArmRadius) /
                    safeArmRadialSpan,
                    0.0f,
                    1.0f
                );

            const float radius =
                baseRadius;

            const float armCenterAngle =
                ComputeSpiralArmAngle(
                    galaxyParameters,
                    armIndex,
                    radius
                );

            //
            // Each arm owns one angular sector.
            //
            // Full sector:
            //     2*pi / armCount
            //
            // We use half of that because displacement
            // can occur clockwise or counter-clockwise
            // from the arm centreline.
            //
            const float halfSectorAngle =
                std::numbers::pi_v<float> /
                static_cast<float>(
                    safeArmCount
                    );

            const float angularSample =
                HashParticle01(
                    index * 6u + 1u
                );


            const float signedAngularSample =
                angularSample *
                2.0f -
                1.0f;

            const float angularSign =
                signedAngularSample < 0.0f
                ? -1.0f
                : 1.0f;

            const float populationStyleSample =
                HashParticle01(
                    index * 6u + 2u
                );


            const float interArmFraction =
                std::clamp(
                    std::lerp(
                        interArmFractionInner,
                        interArmFractionOuter,
                        std::pow(
                            std::clamp(
                                std::max(
                                    baseRadius - innerArmRadius,
                                    0.0f
                                ) /
                                safeArmRadialSpan *
                                (1.0f + interArmOuterBias),
                                0.0f,
                                1.0f
                            ),
                            interArmFractionRadiusPower
                        )
                    ),
                    0.0f,
                    1.0f
                );

            const float regularArmHalfSectorAngle =
                std::min(
                    regularArmHalfWidthInnerRadians *
                    (1.0f - normalizedRadius * normalizedRadius),
                    halfSectorAngle
                );

            const float interArmGapHalfAngle =
                std::max(
                    halfSectorAngle -
                    regularArmHalfSectorAngle,
                    0.0f
                );

            const bool isInterArmParticle =
                populationStyleSample <
                interArmFraction;

            const float angularArmOffset =
                isInterArmParticle
                ? angularSign *
                    (regularArmHalfSectorAngle +
                    std::abs(
                        signedAngularSample
                    ) *
                    interArmGapHalfAngle)
                : signedAngularSample *
                    regularArmHalfSectorAngle;


            const float angle =
                armCenterAngle +
                angularArmOffset;


            const float cosAngle =
                std::cos(angle);


            const float sinAngle =
                std::sin(angle);


            ParticleState& particle =
                particles[index];

            const float heightSampleA =
                HashParticle01(
                    index * 6u + 3u
                );


            const float heightSampleB =
                HashParticle01(
                    index * 6u + 4u
                );


            const float centeredHeightSample =
                heightSampleA +
                heightSampleB -
                1.0f;


            const float height =
                centeredHeightSample *
                galaxyParameters.diskHalfThickness;

            particle.position =
                DirectX::XMFLOAT3{
                    cosAngle * radius,
                    height,
                    sinAngle * radius
            };

            const float angularVelocity =
                ComputeGalaxyAngularVelocityAtRadius(
                    galaxyParameters,
                    radius
                );

            const float orbitSpeed =
                radius *
                angularVelocity;

            particle.velocity =
                DirectX::XMFLOAT3{
                    -sinAngle *
                        orbitSpeed,
                    0.0f,
                    cosAngle *
                        orbitSpeed
            };


            particle.age = 0.0f;


            particle.seed =
                HashParticle01(
                    index * 6u + 5u
                );

        }
        return particles;
    }

    void ParticleSystem::CreateStateBuffers(
        ID3D12Device* device,
        ID3D12GraphicsCommandList*
        commandList,
        const std::vector<ParticleState>&
        initialStates)
    {
        if (
            initialStates.size() !=
            ParticleCount)
        {
            throw std::runtime_error(
                "Particle initialization count "
                "does not match ParticleCount"
            );
        }

        const UINT64 bufferSize =
            static_cast<UINT64>(
                ParticleCount
                ) *
            sizeof(ParticleState);

        D3D12_RESOURCE_DESC
            bufferDescription{};

        bufferDescription.Dimension =
            D3D12_RESOURCE_DIMENSION_BUFFER;

        bufferDescription.Alignment = 0;

        bufferDescription.Width =
            bufferSize;

        bufferDescription.Height = 1;

        bufferDescription.DepthOrArraySize = 1;
        bufferDescription.MipLevels = 1;

        bufferDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        bufferDescription.SampleDesc.Count = 1;
        bufferDescription.SampleDesc.Quality = 0;

        bufferDescription.Layout =
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        bufferDescription.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES
            defaultHeapProperties{};

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
                &bufferDescription,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(
                    m_stateBuffers[0]
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create "
            "particle state buffer A"
        );

        core::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDescription,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_PPV_ARGS(
                    m_stateBuffers[1]
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create "
            "particle state buffer B"
        );

        core::ThrowIfFailed(
            m_stateBuffers[0]->SetName(
                L"Gravity Forge Particle State A"
            ),
            "Failed to name "
            "particle state buffer A"
        );

        core::ThrowIfFailed(
            m_stateBuffers[1]->SetName(
                L"Gravity Forge Particle State B"
            ),
            "Failed to name "
            "particle state buffer B"
        );

        D3D12_HEAP_PROPERTIES
            uploadHeapProperties{};

        uploadHeapProperties.Type =
            D3D12_HEAP_TYPE_UPLOAD;

        uploadHeapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        uploadHeapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        uploadHeapProperties.CreationNodeMask = 1;
        uploadHeapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC
            uploadDescription =
            bufferDescription;

        uploadDescription.Flags =
            D3D12_RESOURCE_FLAG_NONE;

        core::ThrowIfFailed(
            device->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(
                    m_initialUploadBuffer
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create "
            "particle initialization upload buffer"
        );

        void* mappedMemory = nullptr;

        D3D12_RANGE readRange{
            0,
            0
        };

        core::ThrowIfFailed(
            m_initialUploadBuffer->Map(
                0,
                &readRange,
                &mappedMemory
            ),
            "Failed to map "
            "particle initialization upload buffer"
        );

        std::memcpy(
            mappedMemory,
            initialStates.data(),
            static_cast<std::size_t>(
                bufferSize
                )
        );

        D3D12_RANGE writtenRange{
            0,
            static_cast<SIZE_T>(
                bufferSize
            )
        };

        m_initialUploadBuffer->Unmap(
            0,
            &writtenRange
        );

        commandList->CopyBufferRegion(
            m_stateBuffers[0].Get(),
            0,
            m_initialUploadBuffer.Get(),
            0,
            bufferSize
        );

        D3D12_RESOURCE_BARRIER
            initialStateBarrier{};

        initialStateBarrier.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        initialStateBarrier.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        initialStateBarrier
            .Transition
            .pResource =
            m_stateBuffers[0].Get();

        initialStateBarrier
            .Transition
            .Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        initialStateBarrier
            .Transition
            .StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;

        initialStateBarrier
            .Transition
            .StateAfter =
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(
            1,
            &initialStateBarrier
        );
    }

    void ParticleSystem::ReleaseUploadResource()
        noexcept
    {
        m_initialUploadBuffer.Reset();
    }
}
