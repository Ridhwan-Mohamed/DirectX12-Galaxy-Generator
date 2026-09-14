#pragma once

#include "graphics/mesh/MeshData.h"

#include <cstdint>

namespace gf::graphics::mesh
{
    [[nodiscard]]
    MeshData CreateTorusMesh(
        float majorRadius,
        float minorRadius,
        std::uint32_t majorSegments,
        std::uint32_t minorSegments
    );

    [[nodiscard]]
    MeshData CreateUvSphereMesh(
        float radius,
        std::uint32_t latitudeSegments,
        std::uint32_t longitudeSegments
    );

    [[nodiscard]]
    MeshData CreateBoxMesh();
}