#include "Mesh.h"

#include "../../core/HResult.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace
{
    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateBuffer(
            ID3D12Device* device,
            std::uint64_t byteCount,
            D3D12_HEAP_TYPE heapType,
            D3D12_RESOURCE_STATES initialState)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};

        heapProperties.Type = heapType;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_BUFFER;

        description.Alignment = 0;
        description.Width = byteCount;
        description.Height = 1;
        description.DepthOrArraySize = 1;
        description.MipLevels = 1;
        description.Format = DXGI_FORMAT_UNKNOWN;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        description.Flags =
            D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            buffer;

        gf::core::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                initialState,
                nullptr,
                IID_PPV_ARGS(
                    buffer.ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create mesh buffer"
        );

        return buffer;
    }

    void WriteUploadBuffer(
        ID3D12Resource* uploadBuffer,
        const void* sourceData,
        std::size_t byteCount)
    {
        void* mappedMemory = nullptr;

        const D3D12_RANGE cpuReadRange{
            0,
            0
        };

        gf::core::ThrowIfFailed(
            uploadBuffer->Map(
                0,
                &cpuReadRange,
                &mappedMemory
            ),
            "Failed to map mesh upload buffer"
        );

        std::memcpy(
            mappedMemory,
            sourceData,
            byteCount
        );

        const D3D12_RANGE cpuWrittenRange{
            0,
            byteCount
        };

        uploadBuffer->Unmap(
            0,
            &cpuWrittenRange
        );
    }
}

namespace gf::graphics::mesh
{
    Mesh::Mesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        std::span<const Vertex> vertices,
        std::span<const std::uint16_t> indices,
        std::wstring_view debugName)
    {
        if (debugName.empty())
        {
            throw std::invalid_argument(
                "Mesh requires a debug name"
            );
        }

        if (device == nullptr ||
            commandList == nullptr)
        {
            throw std::invalid_argument(
                "Mesh requires a device "
                "and command list"
            );
        }

        if (vertices.empty() ||
            indices.empty())
        {
            throw std::invalid_argument(
                "Mesh requires vertex and index data"
            );
        }

        const std::size_t vertexByteCount = vertices.size_bytes();

        const std::size_t indexByteCount = indices.size_bytes();

        if (vertexByteCount >
            std::numeric_limits<UINT>::max() ||
            indexByteCount >
            std::numeric_limits<UINT>::max() ||
            indices.size() >
            std::numeric_limits<UINT>::max())
        {
            throw std::length_error(
                "Mesh data is too large"
            );
        }

        m_vertexBuffer = CreateBuffer(
            device,
            vertexByteCount,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_COPY_DEST
        );

        m_vertexUploadBuffer = CreateBuffer(
            device,
            vertexByteCount,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );

        WriteUploadBuffer(
            m_vertexUploadBuffer.Get(),
            vertices.data(),
            vertexByteCount
        );

        commandList->CopyBufferRegion(
            m_vertexBuffer.Get(),
            0,
            m_vertexUploadBuffer.Get(),
            0,
            vertexByteCount
        );

        m_indexBuffer = CreateBuffer(
            device,
            indexByteCount,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_COPY_DEST
        );

        m_indexUploadBuffer = CreateBuffer(
            device,
            indexByteCount,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );

        WriteUploadBuffer(
            m_indexUploadBuffer.Get(),
            indices.data(),
            indexByteCount
        );

        commandList->CopyBufferRegion(
            m_indexBuffer.Get(),
            0,
            m_indexUploadBuffer.Get(),
            0,
            indexByteCount
        );

        D3D12_RESOURCE_BARRIER barriers[2]{};

        barriers[0].Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        barriers[0].Transition.pResource =
            m_vertexBuffer.Get();

        barriers[0].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;

        barriers[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

        barriers[1].Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        barriers[1].Transition.pResource =
            m_indexBuffer.Get();

        barriers[1].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;

        barriers[1].Transition.StateAfter =
            D3D12_RESOURCE_STATE_INDEX_BUFFER;

        commandList->ResourceBarrier(
            2,
            barriers
        );

        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();

        m_vertexBufferView.SizeInBytes = static_cast<UINT>(vertexByteCount);

        m_vertexBufferView.StrideInBytes = sizeof(Vertex);

        m_indexBufferView.BufferLocation =
            m_indexBuffer->GetGPUVirtualAddress();

        m_indexBufferView.SizeInBytes =
            static_cast<UINT>(
                indexByteCount
                );

        m_indexBufferView.Format =
            DXGI_FORMAT_R16_UINT;

        m_indexCount =
            static_cast<UINT>(
                indices.size()
                );

        const std::wstring meshName{
            debugName
        };

        const std::wstring vertexBufferName =
            meshName + L" Vertex Buffer";

        const std::wstring indexBufferName =
            meshName + L" Index Buffer";

        const std::wstring vertexUploadName =
            meshName + L" Vertex Upload";

        const std::wstring indexUploadName =
            meshName + L" Index Upload";

        core::ThrowIfFailed(
            m_vertexBuffer->SetName(
                vertexBufferName.c_str()
            ),
            "Failed to name vertex buffer"
        );

        core::ThrowIfFailed(
            m_indexBuffer->SetName(
                indexBufferName.c_str()
            ),
            "Failed to name index buffer"
        );

        core::ThrowIfFailed(
            m_vertexUploadBuffer->SetName(
                vertexUploadName.c_str()
            ),
            "Failed to name vertex upload buffer"
        );

        core::ThrowIfFailed(
            m_indexUploadBuffer->SetName(
                indexUploadName.c_str()
            ),
            "Failed to name index upload buffer"
        );
    }

    void Mesh::ReleaseUploadResources() noexcept
    {
        m_vertexUploadBuffer.Reset();
        m_indexUploadBuffer.Reset();
    }
}