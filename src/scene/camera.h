#pragma once

#include <DirectXMath.h>
#include <numbers>
#include <cstdint>

namespace gf::scene
{
    struct CameraInput
    {
        bool moveForward = false;
        bool moveBackward = false;

        bool moveLeft = false;
        bool moveRight = false;

        bool moveUp = false;
        bool moveDown = false;

        bool turnLeft = false;
        bool turnRight = false;

        bool lookUp = false;
        bool lookDown = false;

        bool rollLeft = false;
        bool rollRight = false;
    };

    class Camera final
    {
    public:
        Camera(
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight
        );

        void SetAspectRatio(
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight
        );

        void SetPose(
            const DirectX::XMFLOAT3& position,
            float yaw,
            float pitch,
            float roll = 0.0f
        );

        void Update(
            const CameraInput& input,
            float deltaSeconds
        );

        [[nodiscard]]
        DirectX::XMMATRIX
            ViewMatrix() const noexcept;

        [[nodiscard]]
        DirectX::XMMATRIX
            ProjectionMatrix() const noexcept;

        [[nodiscard]]
        DirectX::XMVECTOR
            RightVector() const noexcept;

        [[nodiscard]]
        DirectX::XMVECTOR
            UpVector() const noexcept;

    private:
        [[nodiscard]]
        DirectX::XMVECTOR
            ForwardVector() const noexcept;

        DirectX::XMFLOAT3 m_position{
            0.0f,
            0.0f,
            -4.0f
        };

        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        float m_roll = 0.0f;

        float m_aspectRatio = 16.0f / 9.0f;

        float m_verticalFieldOfView = std::numbers::pi_v<float> / 3.0f;

        float m_nearPlane = 0.1f;
        float m_farPlane = 100.0f;
    };
}
