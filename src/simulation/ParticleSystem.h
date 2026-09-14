#pragma once

#include "ParticleState.h"
#include "GalaxyParameters.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <vector>

namespace gf::simulation
{
    class ParticleSystem final
    {
    public:
        static constexpr std::uint32_t
            ParticleCount = 65'536;

        static constexpr std::uint32_t
            StateBufferCount = 2;

        ParticleSystem(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* commandList,
            const GalaxyParameters& galaxyParameters
        );

        ~ParticleSystem() = default;

        ParticleSystem(
            const ParticleSystem&
        ) = delete;

        ParticleSystem& operator=(
            const ParticleSystem&
            ) = delete;

        ParticleSystem(
            ParticleSystem&&
        ) = delete;

        ParticleSystem& operator=(
            ParticleSystem&&
            ) = delete;

        void ReleaseUploadResource() noexcept;

        [[nodiscard]]
        std::uint32_t Count() const noexcept
        {
            return ParticleCount;
        }

        [[nodiscard]]
        ID3D12Resource*
            StateBuffer(
                std::uint32_t index
            ) const noexcept
        {
            if (index >= StateBufferCount)
            {
                return nullptr;
            }

            return m_stateBuffers[index].Get();
        }

        [[nodiscard]]
        ID3D12Resource*
            ReadBuffer() const noexcept
        {
            return
                m_stateBuffers[
                    m_readBufferIndex
                ].Get();
        }

        [[nodiscard]]
        ID3D12Resource*
            WriteBuffer() const noexcept
        {
            return
                m_stateBuffers[
                    m_writeBufferIndex
                ].Get();
        }

        [[nodiscard]]
        std::uint32_t
            ReadBufferIndex() const noexcept
        {
            return m_readBufferIndex;
        }

        [[nodiscard]]
        std::uint32_t
            WriteBufferIndex() const noexcept
        {
            return m_writeBufferIndex;
        }

        void SwapBuffers() noexcept
        {
            const std::uint32_t previousReadIndex =
                m_readBufferIndex;

            m_readBufferIndex =
                m_writeBufferIndex;

            m_writeBufferIndex =
                previousReadIndex;
        }
    private:
        static std::vector<ParticleState>
            CreateInitialStates(
                const GalaxyParameters& galaxyParameters
            );

        void CreateStateBuffers(
            ID3D12Device* device,
            ID3D12GraphicsCommandList*
            commandList,
            const std::vector<ParticleState>&
            initialStates
        );

        std::array<
            Microsoft::WRL::ComPtr<
            ID3D12Resource
            >,
            StateBufferCount
        > m_stateBuffers;

        Microsoft::WRL::ComPtr<
            ID3D12Resource
        > m_initialUploadBuffer;

        std::uint32_t
            m_readBufferIndex = 0;

        std::uint32_t
            m_writeBufferIndex = 1;
    };
}