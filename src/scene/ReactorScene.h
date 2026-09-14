#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <span>
#include <vector>

namespace gf::scene
{
    enum class ReactorMeshKind : std::uint8_t
    {
        Ring,
        Core
    };

    enum class ReactorMaterialKind : std::uint8_t
    {
        Metal,
        Core
    };

    enum class ReactorPipelineKind : std::uint8_t
    {
        Surface,
        Emissive
    };

    struct ReactorPart final
    {
        ReactorMeshKind mesh =
            ReactorMeshKind::Ring;

        ReactorMaterialKind material =
            ReactorMaterialKind::Metal;

        ReactorPipelineKind pipeline =
            ReactorPipelineKind::Surface;

        DirectX::XMFLOAT3 scale{
            1.0f,
            1.0f,
            1.0f
        };

        DirectX::XMFLOAT3 rotation{
            0.0f,
            0.0f,
            0.0f
        };

        DirectX::XMFLOAT3 position{
            0.0f,
            0.0f,
            0.0f
        };

        DirectX::XMFLOAT3 angularVelocity{
            0.0f,
            0.0f,
            0.0f
        };
    };

    class ReactorScene final
    {
    public:
        ReactorScene();

        [[nodiscard]]
        std::span<const ReactorPart>
            Parts() const noexcept
        {
            return m_parts;
        }

    private:
        std::vector<ReactorPart> m_parts;
    };

    [[nodiscard]]
    DirectX::XMMATRIX BuildReactorPartModel(
        const ReactorPart& part,
        float elapsedSeconds
    ) noexcept;
}
