#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

namespace gf::graphics::material
{
    struct Material final
    {
        DirectX::XMFLOAT4 colorTint{
            1.0f,
            1.0f,
            1.0f,
            1.0f
        };

        float metallic = 0.0f;
        float roughness = 1.0f;

        float emissiveIntensity = 0.0f;
        float pulseSpeed = 1.0f;

        D3D12_GPU_DESCRIPTOR_HANDLE
            baseColorSrv{};
    };
}