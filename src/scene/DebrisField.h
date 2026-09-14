#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>
#include <vector>

namespace gf::scene
{
    struct alignas(16) DebrisInstance
    {
        // x = radius, y = phase, z = angular speed, w = height.
        DirectX::XMFLOAT4 orbit;

        // Quaternion used to tilt the orbit plane.
        DirectX::XMFLOAT4 orbitOrientation;

        // Initial mesh orientation quaternion.
        DirectX::XMFLOAT4 localOrientation;

        // xyz = non-uniform scale.
        // w = final world-space bounding-sphere radius.
        DirectX::XMFLOAT4 scaleAndBoundRadius;

        // xyz = tumble axis, w = tumble speed.
        DirectX::XMFLOAT4 tumble;
    };

    static_assert(sizeof(DebrisInstance) == 80);
    static_assert(alignof(DebrisInstance) == 16);

    class DebrisField final
    {
    public:
        static constexpr std::uint32_t InstanceCount = 512;

        DebrisField(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* commandList
        );

        DebrisField(const DebrisField&) = delete;
        DebrisField& operator=(const DebrisField&) = delete;
        DebrisField(DebrisField&&) = delete;
        DebrisField& operator=(DebrisField&&) = delete;

        [[nodiscard]]
        ID3D12Resource* InstanceBuffer() const noexcept
        {
            return m_instanceBuffer.Get();
        }

        [[nodiscard]]
        std::uint32_t Count() const noexcept
        {
            return InstanceCount;
        }

        void ReleaseUploadResource() noexcept;

    private:
        static std::vector<DebrisInstance>
            CreateInitialInstances();

        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_instanceBuffer;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_uploadBuffer;
    };
}