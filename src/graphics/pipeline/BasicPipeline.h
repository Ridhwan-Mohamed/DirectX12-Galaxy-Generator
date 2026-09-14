#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace gf::graphics::pipeline
{

    enum class PipelineKind : std::uint8_t
    {
        Surface,
        Emissive,
        Debris
    };

    class BasicPipeline final
    {
    public:
        BasicPipeline(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat
        ); 
        
        BasicPipeline(
            ID3D12Device* device
        );

        ~BasicPipeline() = default;

        BasicPipeline(const BasicPipeline&) = delete;

        BasicPipeline& operator=(
            const BasicPipeline&
            ) = delete;

        BasicPipeline(
            BasicPipeline&&
        ) = delete;

        BasicPipeline& operator=(
            BasicPipeline&&
            ) = delete;

        [[nodiscard]]
        ID3D12RootSignature*
            RootSignature() const noexcept
        {
            return m_rootSignature.Get();
        }

        [[nodiscard]]
        ID3D12PipelineState* PipelineState(
            bool depthEnabled,
            PipelineKind kind
        ) const noexcept
        {
            if (kind == PipelineKind::Debris)
            {
                return depthEnabled
                    ? m_debrisDepthEnabledPipelineState.Get()
                    : m_debrisDepthDisabledPipelineState.Get();
            }

            if (kind == PipelineKind::Emissive)
            {
                return depthEnabled
                    ? m_emissiveDepthEnabledPipelineState.Get()
                    : m_emissiveDepthDisabledPipelineState.Get();
            }

            return depthEnabled
                ? m_depthEnabledPipelineState.Get()
                : m_depthDisabledPipelineState.Get();
        }

    private:
        void CreateRootSignature(
            ID3D12Device* device
        );

        void CreatePipelineStates(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat
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

        Microsoft::WRL::ComPtr<
            ID3D12PipelineState
        > m_emissiveDepthEnabledPipelineState;

        Microsoft::WRL::ComPtr<
            ID3D12PipelineState
        > m_emissiveDepthDisabledPipelineState;

        Microsoft::WRL::ComPtr<ID3D12PipelineState>
            m_debrisDepthEnabledPipelineState;

        Microsoft::WRL::ComPtr<ID3D12PipelineState>
            m_debrisDepthDisabledPipelineState;
    };
}