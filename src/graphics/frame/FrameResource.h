#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>

namespace gf::graphics::frame
{
    struct SceneConstants
    {
        DirectX::XMFLOAT4X4
            modelViewProjection;

        DirectX::XMFLOAT4 colorTint;

        DirectX::XMFLOAT2 materialParameters;

        DirectX::XMFLOAT2 padding;

        DirectX::XMFLOAT4 animationParameters;
    };

    struct ParticleRenderConstants
    {
        DirectX::XMFLOAT4X4
            viewProjection;

        DirectX::XMFLOAT3 cameraRight;
        float particleHalfSize;

        DirectX::XMFLOAT3 cameraUp;
        float elapsedSeconds;

        DirectX::XMFLOAT4 color;

        //
        // x = core radius
        // y = core particle fraction
        // z = center size multiplier
        // w = center intensity multiplier
        //
        DirectX::XMFLOAT4 coreParameters;

        //
        // xyz = core stellar colour
        // w   = unused
        //
        DirectX::XMFLOAT4 coreColor;


        //
        // xyz = outer-arm stellar colour
        // w   = unused
        //
        DirectX::XMFLOAT4 outerArmColor;


        //
        // x = outer arm radius
        // y = colour-gradient exponent
        // z = colour-jitter strength
        // w = unused
        //
        DirectX::XMFLOAT4 colorParameters;

        DirectX::XMFLOAT4 toneParameters;

        DirectX::XMFLOAT4 starParameters;
    };

    static_assert(
        sizeof(ParticleRenderConstants) == 208
    );

    static_assert(
        sizeof(SceneConstants) == 112
        );

    class FrameResource final
    {
    public:
        explicit FrameResource(ID3D12Device* device, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, std::uint32_t frameIndex);
        ~FrameResource();

        FrameResource(const FrameResource&) = delete;
        FrameResource& operator=(const FrameResource&) = delete;

        FrameResource(FrameResource&& other) noexcept;

        FrameResource& operator=(FrameResource&&) = delete;

        [[nodiscard]]
        ID3D12CommandAllocator*
            CommandAllocator() const noexcept
        {
            return m_commandAllocator.Get();
        }

        [[nodiscard]]
        std::uint64_t FenceValue() const noexcept
        {
            return m_fenceValue;
        }

        void SetFenceValue(
            std::uint64_t value
        ) noexcept
        {
            m_fenceValue = value;
        }

        void WriteSceneConstants(
            std::uint32_t objectIndex,
            const SceneConstants& constants
        ) noexcept;

        void WriteParticleRenderConstants(
            std::uint32_t objectIndex,
            const ParticleRenderConstants& constants
        ) noexcept;

        [[nodiscard]]
        D3D12_GPU_VIRTUAL_ADDRESS
            SceneConstantBufferAddress(
                std::uint32_t objectIndex
        ) const noexcept;

        static constexpr std::uint32_t MaxSceneObjects = 32;

        static constexpr std::uint64_t SceneConstantBufferStride = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_sceneConstantBuffer;

        std::byte* m_mappedSceneConstants = nullptr;

        std::uint64_t m_fenceValue = 0;
    };
}