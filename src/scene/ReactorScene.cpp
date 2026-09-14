#include "ReactorScene.h"

#include <cmath>
#include <numbers>

namespace gf::scene
{
    ReactorScene::ReactorScene()
    {
        m_parts.reserve(4);

    //    m_parts.push_back(
    //        ReactorPart{
    //            .mesh =
    //                ReactorMeshKind::Core,

    //            .material =
    //                ReactorMaterialKind::Core,

    //            .pipeline =
    //                ReactorPipelineKind::Emissive,

    //            .scale = {
    //                1.0f,
    //                1.0f,
    //                1.0f
    //            },

    //            .rotation = {
    //                0.0f,
    //                0.0f,
    //                0.0f
    //            },

    //            .position = {
    //                0.0f,
    //                0.0f,
    //                0.0f
    //            },

    //            .angularVelocity = {
    //                0.0f,
    //                -0.35f,
    //                0.0f
    //            }
    //        }
    //    );

    //    constexpr float pi =
    //        std::numbers::pi_v<float>;

    //    m_parts.push_back(
    //        ReactorPart{
    //            .mesh =
    //                ReactorMeshKind::Ring,

    //            .material =
    //                ReactorMaterialKind::Metal,

    //            .scale = {
    //                1.0f,
    //                1.0f,
    //                1.0f
    //            },

    //            .rotation = {
    //                0.0f,
    //                0.0f,
    //                0.0f
    //            },

    //            .position = {
    //                0.0f,
    //                0.0f,
    //                0.0f
    //            },

    //            .angularVelocity = {
    //                0.0f,
    //                0.0f,
    //                0.22f
    //            }
    //        }
    //    );

    //    m_parts.push_back(
    //        ReactorPart{
    //            .mesh =
    //                ReactorMeshKind::Ring,

    //            .material =
    //                ReactorMaterialKind::Metal,

    //            .scale = {
    //                0.88f,
    //                0.88f,
    //                0.88f
    //            },

    //            .rotation = {
    //                pi * 0.34f,
    //                0.0f,
    //                0.0f
    //            },

    //            .position = {
    //                0.0f,
    //                0.0f,
    //                0.0f
    //            },

    //            .angularVelocity = {
    //                0.0f,
    //                0.18f,
    //                0.0f
    //            }
    //        }
    //    );

    //    m_parts.push_back(
    //        ReactorPart{
    //            .mesh =
    //                ReactorMeshKind::Ring,

    //            .material =
    //                ReactorMaterialKind::Metal,

    //            .scale = {
    //                1.16f,
    //                1.16f,
    //                1.16f
    //            },

    //            .rotation = {
    //                0.0f,
    //                pi * 0.38f,
    //                0.0f
    //            },

    //            .position = {
    //                0.0f,
    //                0.0f,
    //                0.0f
    //            },

    //            .angularVelocity = {
    //                0.14f,
    //                0.0f,
    //                0.0f
    //            }
    //        }
    //    );

    }

    DirectX::XMMATRIX BuildReactorPartModel(
        const ReactorPart& part,
        float elapsedSeconds) noexcept
    {
        const float pitch =
            part.rotation.x +
            part.angularVelocity.x *
            elapsedSeconds;

        const float yaw =
            part.rotation.y +
            part.angularVelocity.y *
            elapsedSeconds;

        const float roll =
            part.rotation.z +
            part.angularVelocity.z *
            elapsedSeconds;

        return
            DirectX::XMMatrixScaling(
                part.scale.x,
                part.scale.y,
                part.scale.z
            )
            *
            DirectX::XMMatrixRotationRollPitchYaw(
                pitch,
                yaw,
                roll
            )
            *
            DirectX::XMMatrixTranslation(
                part.position.x,
                part.position.y,
                part.position.z
            );
    }
}
