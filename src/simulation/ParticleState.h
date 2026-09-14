#pragma once

#include <DirectXMath.h>

namespace gf::simulation
{
    struct ParticleState
    {
        DirectX::XMFLOAT3 position{};
        float age = 0.0f;

        DirectX::XMFLOAT3 velocity{};
        float seed = 0.0f;
    };

    static_assert(
        sizeof(ParticleState) == 32,
        "ParticleState must remain 32 bytes"
        );
}