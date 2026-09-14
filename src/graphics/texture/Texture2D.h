#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string_view>
#include <cstdint>

namespace gf::graphics::texture
{
    struct Rgba8Image;

    class Texture2D final
    {
    public:
        Texture2D(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* commandList,
            const Rgba8Image& image,
            std::wstring_view debugName
        );

        ~Texture2D() = default;

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        Texture2D(Texture2D&&) = delete;
        Texture2D& operator=(Texture2D&&) = delete;

        [[nodiscard]]
        ID3D12Resource* Resource() const noexcept
        {
            return m_texture.Get();
        }

        [[nodiscard]]
        DXGI_FORMAT Format() const noexcept
        {
            return m_format;
        }

        [[nodiscard]]
        std::uint32_t Width() const noexcept
        {
            return m_width;
        }

        [[nodiscard]]
        std::uint32_t Height() const noexcept
        {
            return m_height;
        }

        void ReleaseUploadResource() noexcept;

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_texture;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_uploadBuffer;

        DXGI_FORMAT m_format =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
    };
}