#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace gf::graphics::pipeline
{
    class PresentationPipeline final
    {
    public:
        PresentationPipeline(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat
        );

        ~PresentationPipeline() = default;

        PresentationPipeline(
            const PresentationPipeline&
        ) = delete;

        PresentationPipeline& operator=(
            const PresentationPipeline&
            ) = delete;

        PresentationPipeline(
            PresentationPipeline&&
        ) = delete;

        PresentationPipeline& operator=(
            PresentationPipeline&&
            ) = delete;

        [[nodiscard]]
        ID3D12RootSignature*
            RootSignature() const noexcept
        {
            return m_rootSignature.Get();
        }

        [[nodiscard]]
        ID3D12PipelineState*
            PipelineState() const noexcept
        {
            return m_pipelineState.Get();
        }

    private:
        void CreateRootSignature(
            ID3D12Device* device
        );

        void CreatePipelineState(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat
        );

        Microsoft::WRL::ComPtr<
            ID3D12RootSignature
        > m_rootSignature;

        Microsoft::WRL::ComPtr<
            ID3D12PipelineState
        > m_pipelineState;
    };
}