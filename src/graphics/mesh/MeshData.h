#pragma once

#include "mesh.h"

#include <cstdint>
#include <vector>

namespace gf::graphics::mesh
{
    struct MeshData final
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint16_t> indices;
    };
}