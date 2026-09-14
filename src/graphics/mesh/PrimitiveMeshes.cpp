#include "graphics/mesh/PrimitiveMeshes.h"

#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace
{
    void ValidateVertexCount(
        std::size_t vertexCount)
    {
        constexpr std::size_t maximumVertexCount =
            static_cast<std::size_t>(
                std::numeric_limits<
                std::uint16_t
                >::max()
                ) + 1;

        if (vertexCount > maximumVertexCount)
        {
            throw std::length_error(
                "Primitive contains too many vertices "
                "for 16-bit indices"
            );
        }
    }
}

namespace gf::graphics::mesh
{
    MeshData CreateTorusMesh(
        float majorRadius,
        float minorRadius,
        std::uint32_t majorSegments,
        std::uint32_t minorSegments)
    {
        if (majorRadius <= 0.0f ||
            minorRadius <= 0.0f)
        {
            throw std::invalid_argument(
                "Torus radii must be positive"
            );
        }

        if (majorSegments < 3 ||
            minorSegments < 3)
        {
            throw std::invalid_argument(
                "Torus requires at least three "
                "segments around each axis"
            );
        }

        const std::size_t majorVertexCount =
            static_cast<std::size_t>(
                majorSegments
                ) + 1;

        const std::size_t minorVertexCount =
            static_cast<std::size_t>(
                minorSegments
                ) + 1;

        const std::size_t totalVertexCount =
            majorVertexCount *
            minorVertexCount;

        ValidateVertexCount(
            totalVertexCount
        );

        MeshData result;

        result.vertices.reserve(
            totalVertexCount
        );

        result.indices.reserve(
            static_cast<std::size_t>(
                majorSegments
                ) *
            static_cast<std::size_t>(
                minorSegments
                ) *
            6
        );

        constexpr float twoPi =
            2.0f *
            std::numbers::pi_v<float>;

        // Stage 1: Generate every vertex.
        for (
            std::uint32_t majorIndex = 0;
            majorIndex <= majorSegments;
            ++majorIndex)
        {
            const float u =
                static_cast<float>(majorIndex) /
                static_cast<float>(majorSegments);

            const float majorAngle =
                u * twoPi;

            const float majorCosine =
                std::cos(majorAngle);

            const float majorSine =
                std::sin(majorAngle);

            for (
                std::uint32_t minorIndex = 0;
                minorIndex <= minorSegments;
                ++minorIndex)
            {
                const float v =
                    static_cast<float>(minorIndex) /
                    static_cast<float>(minorSegments);

                const float minorAngle =
                    v * twoPi;

                const float minorCosine =
                    std::cos(minorAngle);

                const float minorSine =
                    std::sin(minorAngle);

                const float ringDistance =
                    majorRadius +
                    minorRadius *
                    minorCosine;

                const float x =
                    ringDistance *
                    majorCosine;

                const float y =
                    ringDistance *
                    majorSine;

                const float z =
                    minorRadius *
                    minorSine;

                result.vertices.push_back(
                    Vertex{
                        DirectX::XMFLOAT3{
                            x,
                            y,
                            z
                        },
                        DirectX::XMFLOAT3{
                            1.0f,
                            1.0f,
                            1.0f
                        },
                        DirectX::XMFLOAT2{
                            u,
                            v
                        }
                    }
                );
            }
        }

        // Stage 2: Connect those vertices with indices.
        for (
            std::uint32_t majorIndex = 0;
            majorIndex < majorSegments;
            ++majorIndex)
        {
            for (
                std::uint32_t minorIndex = 0;
                minorIndex < minorSegments;
                ++minorIndex)
            {
                const std::size_t current =
                    static_cast<std::size_t>(
                        majorIndex
                        ) *
                    minorVertexCount +
                    minorIndex;

                const std::size_t nextMajor =
                    static_cast<std::size_t>(
                        majorIndex + 1
                        ) *
                    minorVertexCount +
                    minorIndex;

                const std::uint16_t i0 =
                    static_cast<std::uint16_t>(
                        current
                        );

                const std::uint16_t i1 =
                    static_cast<std::uint16_t>(
                        nextMajor
                        );

                const std::uint16_t i2 =
                    static_cast<std::uint16_t>(
                        nextMajor + 1
                        );

                const std::uint16_t i3 =
                    static_cast<std::uint16_t>(
                        current + 1
                        );

                result.indices.push_back(i0);
                result.indices.push_back(i1);
                result.indices.push_back(i2);

                result.indices.push_back(i0);
                result.indices.push_back(i2);
                result.indices.push_back(i3);
            }
        }

        // Only return after all vertices and indices exist.
        return result;
    }

    MeshData CreateUvSphereMesh(
        float radius,
        std::uint32_t latitudeSegments,
        std::uint32_t longitudeSegments)
    {
        if (radius <= 0.0f)
        {
            throw std::invalid_argument(
                "Sphere radius must be positive"
            );
        }

        if (latitudeSegments < 2 ||
            longitudeSegments < 3)
        {
            throw std::invalid_argument(
                "Sphere has insufficient segments"
            );
        }

        const std::size_t latitudeVertexCount =
            static_cast<std::size_t>(
                latitudeSegments
                ) + 1;

        const std::size_t longitudeVertexCount =
            static_cast<std::size_t>(
                longitudeSegments
                ) + 1;

        const std::size_t totalVertexCount =
            latitudeVertexCount *
            longitudeVertexCount;

        ValidateVertexCount(
            totalVertexCount
        );

        MeshData result;

        result.vertices.reserve(
            totalVertexCount
        );

        result.indices.reserve(
            static_cast<std::size_t>(
                latitudeSegments
                ) *
            static_cast<std::size_t>(
                longitudeSegments
                ) *
            6
        );

        constexpr float pi =
            std::numbers::pi_v<float>;

        constexpr float twoPi =
            2.0f * pi;

        for (
            std::uint32_t latitudeIndex = 0;
            latitudeIndex <= latitudeSegments;
            ++latitudeIndex)
        {
            const float v =
                static_cast<float>(latitudeIndex) /
                static_cast<float>(latitudeSegments);

            const float latitudeAngle =
                v * pi;

            const float latitudeSine =
                std::sin(latitudeAngle);

            const float latitudeCosine =
                std::cos(latitudeAngle);

            for (
                std::uint32_t longitudeIndex = 0;
                longitudeIndex <= longitudeSegments;
                ++longitudeIndex)
            {
                const float u =
                    static_cast<float>(longitudeIndex) /
                    static_cast<float>(longitudeSegments);

                const float longitudeAngle =
                    u * twoPi;

                const float longitudeCosine =
                    std::cos(longitudeAngle);

                const float longitudeSine =
                    std::sin(longitudeAngle);

                const float x =
                    radius *
                    latitudeSine *
                    longitudeCosine;

                const float y =
                    radius *
                    latitudeCosine;

                const float z =
                    radius *
                    latitudeSine *
                    longitudeSine;

                result.vertices.push_back(
                    Vertex{
                        DirectX::XMFLOAT3{
                            x,
                            y,
                            z
                        },
                        DirectX::XMFLOAT3{
                            1.0f,
                            1.0f,
                            1.0f
                        },
                        DirectX::XMFLOAT2{
                            u,
                            v
                        }
                    }
                );
            }
        }

        for (
            std::uint32_t latitudeIndex = 0;
            latitudeIndex < latitudeSegments;
            ++latitudeIndex)
        {
            for (
                std::uint32_t longitudeIndex = 0;
                longitudeIndex < longitudeSegments;
                ++longitudeIndex)
            {
                const std::size_t current =
                    static_cast<std::size_t>(
                        latitudeIndex
                        ) *
                    longitudeVertexCount +
                    longitudeIndex;

                const std::size_t nextLatitude =
                    static_cast<std::size_t>(
                        latitudeIndex + 1
                        ) *
                    longitudeVertexCount +
                    longitudeIndex;

                const std::uint16_t i0 =
                    static_cast<std::uint16_t>(
                        current
                        );

                const std::uint16_t i1 =
                    static_cast<std::uint16_t>(
                        current + 1
                        );

                const std::uint16_t i2 =
                    static_cast<std::uint16_t>(
                        nextLatitude + 1
                        );

                const std::uint16_t i3 =
                    static_cast<std::uint16_t>(
                        nextLatitude
                        );

                result.indices.push_back(i0);
                result.indices.push_back(i1);
                result.indices.push_back(i2);

                result.indices.push_back(i0);
                result.indices.push_back(i2);
                result.indices.push_back(i3);
            }
        }

        return result;
    }

    MeshData CreateBoxMesh()
    {
        MeshData result;

        result.vertices.reserve(24);
        result.indices.reserve(36);

        const auto addFace =
            [&result](
                const DirectX::XMFLOAT3& topLeft,
                const DirectX::XMFLOAT3& topRight,
                const DirectX::XMFLOAT3& bottomRight,
                const DirectX::XMFLOAT3& bottomLeft)
            {
                const std::uint16_t baseIndex =
                    static_cast<std::uint16_t>(
                        result.vertices.size()
                        );

                constexpr DirectX::XMFLOAT3 white{
                    1.0f,
                    1.0f,
                    1.0f
                };

                result.vertices.push_back(
                    Vertex{
                        topLeft,
                        white,
                        DirectX::XMFLOAT2{
                            0.0f,
                            0.0f
                        }
                    }
                );

                result.vertices.push_back(
                    Vertex{
                        topRight,
                        white,
                        DirectX::XMFLOAT2{
                            1.0f,
                            0.0f
                        }
                    }
                );

                result.vertices.push_back(
                    Vertex{
                        bottomRight,
                        white,
                        DirectX::XMFLOAT2{
                            1.0f,
                            1.0f
                        }
                    }
                );

                result.vertices.push_back(
                    Vertex{
                        bottomLeft,
                        white,
                        DirectX::XMFLOAT2{
                            0.0f,
                            1.0f
                        }
                    }
                );

                const std::uint16_t i0 =
                    baseIndex;

                const std::uint16_t i1 =
                    static_cast<std::uint16_t>(
                        baseIndex + 1
                        );

                const std::uint16_t i2 =
                    static_cast<std::uint16_t>(
                        baseIndex + 2
                        );

                const std::uint16_t i3 =
                    static_cast<std::uint16_t>(
                        baseIndex + 3
                        );

                result.indices.push_back(i0);
                result.indices.push_back(i1);
                result.indices.push_back(i2);

                result.indices.push_back(i0);
                result.indices.push_back(i2);
                result.indices.push_back(i3);
            };
            
        constexpr float halfExtent = 0.5f;

        // Front: negative Z.
        addFace(
            { -halfExtent,  halfExtent, -halfExtent },
            { halfExtent,  halfExtent, -halfExtent },
            { halfExtent, -halfExtent, -halfExtent },
            { -halfExtent, -halfExtent, -halfExtent }
        );

        // Back: positive Z.
        addFace(
            { halfExtent,  halfExtent,  halfExtent },
            { -halfExtent,  halfExtent,  halfExtent },
            { -halfExtent, -halfExtent,  halfExtent },
            { halfExtent, -halfExtent,  halfExtent }
        );

        // Right: positive X.
        addFace(
            { halfExtent,  halfExtent, -halfExtent },
            { halfExtent,  halfExtent,  halfExtent },
            { halfExtent, -halfExtent,  halfExtent },
            { halfExtent, -halfExtent, -halfExtent }
        );

        // Left: negative X.
        addFace(
            { -halfExtent,  halfExtent,  halfExtent },
            { -halfExtent,  halfExtent, -halfExtent },
            { -halfExtent, -halfExtent, -halfExtent },
            { -halfExtent, -halfExtent,  halfExtent }
        );

        // Top: positive Y.
        addFace(
            { -halfExtent, halfExtent,  halfExtent },
            { halfExtent, halfExtent,  halfExtent },
            { halfExtent, halfExtent, -halfExtent },
            { -halfExtent, halfExtent, -halfExtent }
        );

        // Bottom: negative Y.
        addFace(
            { -halfExtent, -halfExtent, -halfExtent },
            { halfExtent, -halfExtent, -halfExtent },
            { halfExtent, -halfExtent,  halfExtent },
            { -halfExtent, -halfExtent,  halfExtent }
        );

        return result;
    }
}