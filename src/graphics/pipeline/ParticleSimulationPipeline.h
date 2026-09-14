#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace gf::graphics::pipeline
{
    class ParticleSimulationPipeline final
    {
    public:
        explicit ParticleSimulationPipeline(
            ID3D12Device* device
        );

        ~ParticleSimulationPipeline() = default;

        ParticleSimulationPipeline(
            const ParticleSimulationPipeline&
        ) = delete;

        ParticleSimulationPipeline& operator=(
            const ParticleSimulationPipeline&
            ) = delete;

        ParticleSimulationPipeline(
            ParticleSimulationPipeline&&
        ) = delete;

        ParticleSimulationPipeline& operator=(
            ParticleSimulationPipeline&&
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