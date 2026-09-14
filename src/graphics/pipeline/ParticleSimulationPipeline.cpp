#include "ParticleSimulationPipeline.h"

#include "../../core/HResult.h"

#include "ParticleSimCS.h"

#include <Windows.h>

#include <stdexcept>

namespace gf::graphics::pipeline
{
    ParticleSimulationPipeline::
        ParticleSimulationPipeline(
            ID3D12Device* device)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "ParticleSimulationPipeline requires "
                "a valid D3D12 device"
            );
        }

        CreateRootSignature(device);
        CreatePipelineState(device);
    }

    void ParticleSimulationPipeline::
        CreateRootSignature(
            ID3D12Device* device)
    {
        D3D12_DESCRIPTOR_RANGE
            previousStateRange{};

        previousStateRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        previousStateRange.NumDescriptors = 1;

        previousStateRange.BaseShaderRegister = 0;
        previousStateRange.RegisterSpace = 0;

        previousStateRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


        D3D12_DESCRIPTOR_RANGE
            nextStateRange{};

        nextStateRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

        nextStateRange.NumDescriptors = 1;

        nextStateRange.BaseShaderRegister = 0;
        nextStateRange.RegisterSpace = 0;

        nextStateRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


        D3D12_ROOT_PARAMETER
            parameters[3]{};


        // Root parameter 0:
        // StructuredBuffer<ParticleState> : register(t0)
        parameters[0].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[0]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[0]
            .DescriptorTable
            .pDescriptorRanges =
            &previousStateRange;

        parameters[0].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;


        // Root parameter 1:
        // RWStructuredBuffer<ParticleState> : register(u0)
        parameters[1].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[1]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[1]
            .DescriptorTable
            .pDescriptorRanges =
            &nextStateRange;

        parameters[1].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;


        //
        // Root parameter 2:
        //
        // Simulation control constants.
        // forty 32-bit values bound to b0.
        //
        parameters[2].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

        parameters[2]
            .Constants
            .ShaderRegister = 0;

        parameters[2]
            .Constants
            .RegisterSpace = 0;

        parameters[2]
            .Constants
            .Num32BitValues = 44;

        parameters[2].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;


        D3D12_ROOT_SIGNATURE_DESC
            description{};

        description.NumParameters =
            _countof(parameters);

        description.pParameters =
            parameters;

        description.NumStaticSamplers = 0;
        description.pStaticSamplers = nullptr;

        description.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_NONE;


        Microsoft::WRL::ComPtr<ID3DBlob>
            serializedSignature;

        Microsoft::WRL::ComPtr<ID3DBlob>
            serializationErrors;

        const HRESULT serializationResult =
            D3D12SerializeRootSignature(
                &description,
                D3D_ROOT_SIGNATURE_VERSION_1,
                serializedSignature
                .ReleaseAndGetAddressOf(),
                serializationErrors
                .ReleaseAndGetAddressOf()
            );

        if (serializationErrors != nullptr)
        {
            OutputDebugStringA(
                static_cast<const char*>(
                    serializationErrors
                    ->GetBufferPointer()
                    )
            );

            OutputDebugStringA("\n");
        }

        core::ThrowIfFailed(
            serializationResult,
            "Failed to serialize particle "
            "simulation root signature"
        );

        core::ThrowIfFailed(
            device->CreateRootSignature(
                0,
                serializedSignature
                ->GetBufferPointer(),
                serializedSignature
                ->GetBufferSize(),
                IID_PPV_ARGS(
                    m_rootSignature
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create particle "
            "simulation root signature"
        );

        core::ThrowIfFailed(
            m_rootSignature->SetName(
                L"Gravity Forge "
                L"Particle Simulation Root Signature"
            ),
            "Failed to name particle "
            "simulation root signature"
        );
    }

    void ParticleSimulationPipeline::
        CreatePipelineState(
            ID3D12Device* device)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC
            description{};

        description.pRootSignature =
            m_rootSignature.Get();

        description.CS.pShaderBytecode =
            g_ParticleSimComputeShader;

        description.CS.BytecodeLength =
            sizeof(g_ParticleSimComputeShader);

        description.NodeMask = 0;

        description.Flags =
            D3D12_PIPELINE_STATE_FLAG_NONE;

        core::ThrowIfFailed(
            device->CreateComputePipelineState(
                &description,
                IID_PPV_ARGS(
                    m_pipelineState
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create particle "
            "simulation compute pipeline"
        );

        core::ThrowIfFailed(
            m_pipelineState->SetName(
                L"Gravity Forge "
                L"Particle Simulation Compute Pipeline"
            ),
            "Failed to name particle "
            "simulation compute pipeline"
        );
    }
}