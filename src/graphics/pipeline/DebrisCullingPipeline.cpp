#include "DebrisCullingPipeline.h"

#include "../../core/HResult.h"

#include "DebrisCullCS.h"

#include <Windows.h>
#include <stdexcept>

namespace gf::graphics::pipeline
{
    DebrisCullingPipeline::
        DebrisCullingPipeline(
            ID3D12Device* device)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "DebrisCullingPipeline requires "
                "a valid D3D12 device"
            );
        }

        CreateRootSignature(device);
        CreatePipelineState(device);
    }

    void DebrisCullingPipeline::CreateRootSignature(
        ID3D12Device* device
    )
    {
        D3D12_DESCRIPTOR_RANGE
            debrisInstancesRange{};

        debrisInstancesRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        debrisInstancesRange.NumDescriptors = 1;
        debrisInstancesRange.BaseShaderRegister = 0;
        debrisInstancesRange.RegisterSpace = 0;

        debrisInstancesRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


        D3D12_DESCRIPTOR_RANGE
            visibilityFlagsRange{};

        visibilityFlagsRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

        visibilityFlagsRange.NumDescriptors = 1;
        visibilityFlagsRange.BaseShaderRegister = 0;
        visibilityFlagsRange.RegisterSpace = 0;

        visibilityFlagsRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE
            visibleInstanceIndicesRange{};

        visibleInstanceIndicesRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

        visibleInstanceIndicesRange.NumDescriptors = 1;
        visibleInstanceIndicesRange.BaseShaderRegister = 1;
        visibleInstanceIndicesRange.RegisterSpace = 0;

        visibleInstanceIndicesRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


        D3D12_DESCRIPTOR_RANGE
            visibleCountRange{};

        visibleCountRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

        visibleCountRange.NumDescriptors = 1;
        visibleCountRange.BaseShaderRegister = 2;
        visibleCountRange.RegisterSpace = 0;

        visibleCountRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER parameters[5]{};

        parameters[0].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[0]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[0]
            .DescriptorTable
            .pDescriptorRanges =
            &debrisInstancesRange;

        parameters[0].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;


        parameters[1].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[1]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[1]
            .DescriptorTable
            .pDescriptorRanges =
            &visibilityFlagsRange;

        parameters[1].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;


        parameters[2].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[2]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[2]
            .DescriptorTable
            .pDescriptorRanges =
            &visibleInstanceIndicesRange;

        parameters[2].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;


        parameters[3].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[3]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[3]
            .DescriptorTable
            .pDescriptorRanges =
            &visibleCountRange;

        parameters[3].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;

        parameters[4].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

        parameters[4].Constants.ShaderRegister = 0;
        parameters[4].Constants.RegisterSpace = 0;
        parameters[4].Constants.Num32BitValues = 28;

        parameters[4].ShaderVisibility =
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
            "Failed to serialize debris "
            "culling root signature"
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
            "Failed to create debris "
            "culling root signature"
        );


        m_rootSignature->SetName(
            L"Gravity Forge Debris Culling "
            L"Root Signature"
        );
    }

    void DebrisCullingPipeline::
        CreatePipelineState(
            ID3D12Device* device)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC
            description{};

        description.pRootSignature =
            m_rootSignature.Get();

        description.CS.pShaderBytecode =
            g_DebrisCullComputeShader;

        description.CS.BytecodeLength =
            sizeof(g_DebrisCullComputeShader);

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
            "Failed to create debris culling "
            "compute pipeline"
        );

        core::ThrowIfFailed(
            m_pipelineState->SetName(
                L"Gravity Forge Debris Culling "
                L"Compute Pipeline"
            ),
            "Failed to name debris culling pipeline"
        );
    }
}
