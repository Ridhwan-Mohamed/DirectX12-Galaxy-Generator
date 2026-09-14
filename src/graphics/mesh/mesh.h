#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <string_view>

#include <cstdint>
#include <span>

namespace gf::graphics::mesh
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
        DirectX::XMFLOAT2 uv;
    };

    class Mesh final
    {
    public:
        Mesh(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* commandList,
            std::span<const Vertex> vertices,
            std::span<const std::uint16_t> indices,
            std::wstring_view debugName
        );

        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&&) = delete;
        Mesh& operator=(Mesh&&) = delete;

        [[nodiscard]]
        const D3D12_VERTEX_BUFFER_VIEW&
            VertexBufferView() const noexcept
        {
            return m_vertexBufferView;
        }

        [[nodiscard]]
        const D3D12_INDEX_BUFFER_VIEW&
            IndexBufferView() const noexcept
        {
            return m_indexBufferView;
        }

        [[nodiscard]]
        UINT IndexCount() const noexcept
        {
            return m_indexCount;
        }

        void ReleaseUploadResources() noexcept;

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_vertexBuffer;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_indexBuffer;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_vertexUploadBuffer;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            m_indexUploadBuffer;

        D3D12_VERTEX_BUFFER_VIEW
            m_vertexBufferView{};

        D3D12_INDEX_BUFFER_VIEW
            m_indexBufferView{};

        UINT m_indexCount = 0;
    };
}