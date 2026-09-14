#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace gf::graphics::pipeline
{
    class DebrisCullingPipeline final
    {
    public:
        explicit DebrisCullingPipeline(
            ID3D12Device* device
        );

        DebrisCullingPipeline(
            const DebrisCullingPipeline&
        ) = delete;

        DebrisCullingPipeline& operator=(
            const DebrisCullingPipeline&
            ) = delete;

        DebrisCullingPipeline(
            DebrisCullingPipeline&&
        ) = delete;

        DebrisCullingPipeline& operator=(
            DebrisCullingPipeline&&
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
            ID3D12Device* device
        );

        Microsoft::WRL::ComPtr<
            ID3D12RootSignature
        > m_rootSignature;

        Microsoft::WRL::ComPtr<
            ID3D12PipelineState
        > m_pipelineState;
    };
}