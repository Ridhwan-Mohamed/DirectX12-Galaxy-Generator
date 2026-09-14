#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace gf::graphics::pipeline
{
    class ParticleDebugPipeline final
    {
    public:
        ParticleDebugPipeline(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat,
            DXGI_FORMAT depthFormat
        );

        ~ParticleDebugPipeline() = default;

        ParticleDebugPipeline(
            const ParticleDebugPipeline&
        ) = delete;

        ParticleDebugPipeline& operator=(
            const ParticleDebugPipeline&
            ) = delete;

        ParticleDebugPipeline(
            ParticleDebugPipeline&&
        ) = delete;

        ParticleDebugPipeline& operator=(
            ParticleDebugPipeline&&
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
            DXGI_FORMAT renderTargetFormat,
            DXGI_FORMAT depthFormat
        );


        Microsoft::WRL::ComPtr<
            ID3D12RootSignature
        > m_rootSignature;

        Microsoft::WRL::ComPtr<
            ID3D12PipelineState
        > m_pipelineState;
    };
}