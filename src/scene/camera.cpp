#include "Camera.h"

#include <algorithm>
#include <stdexcept>

namespace gf::scene
{
    Camera::Camera(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight)
    {
        SetAspectRatio(
            viewportWidth,
            viewportHeight
        );
    }

    void Camera::SetAspectRatio(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight)
    {
        if (viewportWidth == 0 ||
            viewportHeight == 0)
        {
            return;
        }

        m_aspectRatio =
            static_cast<float>(viewportWidth) /
            static_cast<float>(viewportHeight);
    }

    void Camera::SetPose(
        const DirectX::XMFLOAT3& position,
        float yaw,
        float pitch,
        float roll)
    {
        m_position = position;

        m_yaw = yaw;
        m_pitch = std::clamp(
            pitch,
            -1.5f,
            1.5f
        );
        m_roll = roll;
    }

    DirectX::XMVECTOR
        Camera::ForwardVector() const noexcept
    {
        const DirectX::XMMATRIX rotation =
            DirectX::XMMatrixRotationRollPitchYaw(
                m_pitch,
                m_yaw,
                m_roll
            );

        const DirectX::XMVECTOR
            defaultForward =
            DirectX::XMVectorSet(
                0.0f,
                0.0f,
                1.0f,
                0.0f
            );

        return DirectX::XMVector3Normalize(
            DirectX::XMVector3TransformNormal(
                defaultForward,
                rotation
            )
        );
    }

    DirectX::XMVECTOR
        Camera::RightVector() const noexcept
    {
        const DirectX::XMMATRIX rotation =
            DirectX::XMMatrixRotationRollPitchYaw(
                m_pitch,
                m_yaw,
                m_roll
            );

        const DirectX::XMVECTOR localRight =
            DirectX::XMVectorSet(
                1.0f,
                0.0f,
                0.0f,
                0.0f
            );

        return DirectX::XMVector3Normalize(
            DirectX::XMVector3TransformNormal(
                localRight,
                rotation
            )
        );
    }


    DirectX::XMVECTOR
        Camera::UpVector() const noexcept
    {
        const DirectX::XMMATRIX rotation =
            DirectX::XMMatrixRotationRollPitchYaw(
                m_pitch,
                m_yaw,
                m_roll
            );

        const DirectX::XMVECTOR localUp =
            DirectX::XMVectorSet(
                0.0f,
                1.0f,
                0.0f,
                0.0f
            );

        return DirectX::XMVector3Normalize(
            DirectX::XMVector3TransformNormal(
                localUp,
                rotation
            )
        );
    }

    void Camera::Update(
        const CameraInput& input,
        float deltaSeconds)
    {
        const float safeDeltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.1f
            );

        constexpr float movementSpeed =
            2.5f;

        constexpr float rotationSpeed =
            1.5f;

        const float yawInput =
            static_cast<float>(input.turnRight) -
            static_cast<float>(input.turnLeft);

        const float pitchInput =
            static_cast<float>(input.lookDown) - 
            static_cast<float>(input.lookUp);

        const float rollInput =
            static_cast<float>(input.rollLeft) -
            static_cast<float>(input.rollRight);

        m_yaw +=
            yawInput *
            rotationSpeed *
            safeDeltaSeconds;

        m_pitch +=
            pitchInput *
            rotationSpeed *
            safeDeltaSeconds;

        m_pitch = std::clamp(
            m_pitch,
            -1.5f,
            1.5f
        );

        m_roll +=
            rollInput *
            rotationSpeed *
            safeDeltaSeconds;

        const DirectX::XMVECTOR forward =
            ForwardVector();

        const DirectX::XMVECTOR right =
            RightVector();

        const float forwardInput =
            static_cast<float>(input.moveForward) -
            static_cast<float>(input.moveBackward);

        const float rightInput =
            static_cast<float>(input.moveRight) -
            static_cast<float>(input.moveLeft);

        const float upInput =
            static_cast<float>(input.moveUp) -
            static_cast<float>(input.moveDown);

        const DirectX::XMVECTOR worldUp =
            DirectX::XMVectorSet(
                0.0f,
                1.0f,
                0.0f,
                0.0f
            );

        DirectX::XMVECTOR movement =
            DirectX::XMVectorZero();

        movement = DirectX::XMVectorAdd(
            movement,
            DirectX::XMVectorScale(
                forward,
                forwardInput
            )
        );

        movement = DirectX::XMVectorAdd(
            movement,
            DirectX::XMVectorScale(
                right,
                rightInput
            )
        );

        movement = DirectX::XMVectorAdd(
            movement,
            DirectX::XMVectorScale(
                worldUp,
                upInput
            )
        );

        const float movementLengthSquared =
            DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(
                    movement
                )
            );

        if (movementLengthSquared > 0.0f)
        {
            movement =
                DirectX::XMVector3Normalize(
                    movement
                );
        }

        DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(
                &m_position
            );

        position = DirectX::XMVectorAdd(
            position,
            DirectX::XMVectorScale(
                movement,
                movementSpeed *
                safeDeltaSeconds
            )
        );

        DirectX::XMStoreFloat3(
            &m_position,
            position
        );
    }

    DirectX::XMMATRIX
        Camera::ViewMatrix() const noexcept
    {
        const DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(
                &m_position
            );

        const DirectX::XMVECTOR worldUp =
            DirectX::XMVectorSet(
                0.0f,
                1.0f,
                0.0f,
                0.0f
            );

        return DirectX::XMMatrixLookToLH(
            position,
            ForwardVector(),
            UpVector()
        );
    }

    DirectX::XMMATRIX
        Camera::ProjectionMatrix() const noexcept
    {
        return DirectX::XMMatrixPerspectiveFovLH(
            m_verticalFieldOfView,
            m_aspectRatio,
            m_nearPlane,
            m_farPlane
        );
    }
}
