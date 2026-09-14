#pragma once

#include <d3d12.h>
#include <wrl/client.h>


namespace gf::graphics::pipeline
{
    class ParticleBillboardPipeline final
    {
    public:
        ParticleBillboardPipeline(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat,
            DXGI_FORMAT depthFormat
        );

        ~ParticleBillboardPipeline() = default;

        ParticleBillboardPipeline(
            const ParticleBillboardPipeline&
        ) = delete;

        ParticleBillboardPipeline& operator=(
            const ParticleBillboardPipeline&
            ) = delete;

        ParticleBillboardPipeline(
            ParticleBillboardPipeline&&
        ) = delete;

        ParticleBillboardPipeline& operator=(
            ParticleBillboardPipeline&&
            ) = delete;


        [[nodiscard]]
        ID3D12RootSignature*
            RootSignature() const noexcept
        {
            return m_rootSignature.Get();
        }


        [[nodiscard]]
        ID3D12PipelineState*
            PipelineState(
                bool depthTestingEnabled
            ) const noexcept
        {
            return depthTestingEnabled
                ? m_depthEnabledPipelineState.Get()
                : m_depthDisabledPipelineState.Get();
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
        > m_depthEnabledPipelineState;

        Microsoft::WRL::ComPtr<
            ID3D12PipelineState
        > m_depthDisabledPipelineState;
    };
}