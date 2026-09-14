#include "BasicPipeline.h"

#include "../../core/HResult.h"

#include "BasicPS.h"
#include "BasicVS.h"
#include "EmissivePS.h"
#include "DebrisVS.h"
#include "DebrisPS.h"

#include <Windows.h>

#include <limits>
#include <stdexcept>

namespace gf::graphics::pipeline
{
    BasicPipeline::BasicPipeline(
        ID3D12Device* device,
        DXGI_FORMAT renderTargetFormat)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "BasicPipeline requires "
                "a valid D3D12 device"
            );
        }

        if (renderTargetFormat ==
            DXGI_FORMAT_UNKNOWN)
        {
            throw std::invalid_argument(
                "BasicPipeline requires "
                "a valid render-target format"
            );
        }

        CreateRootSignature(device);
        CreatePipelineStates(device, renderTargetFormat);
    }

    void BasicPipeline::CreateRootSignature(
        ID3D12Device* device)
    {
        D3D12_DESCRIPTOR_RANGE textureRange{};

        textureRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        textureRange.NumDescriptors = 1;
        textureRange.BaseShaderRegister = 0;
        textureRange.RegisterSpace = 0;

        textureRange.OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE debrisInstancesRange{};

        debrisInstancesRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        debrisInstancesRange.NumDescriptors = 1;
        debrisInstancesRange.BaseShaderRegister = 0;
        debrisInstancesRange.RegisterSpace = 1;

        debrisInstancesRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE
            visibleInstanceIndicesRange{};

        visibleInstanceIndicesRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        visibleInstanceIndicesRange.NumDescriptors = 1;
        visibleInstanceIndicesRange.BaseShaderRegister = 1;
        visibleInstanceIndicesRange.RegisterSpace = 1;

        visibleInstanceIndicesRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


        D3D12_DESCRIPTOR_RANGE
            visibleCountRange{};

        visibleCountRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        visibleCountRange.NumDescriptors = 1;
        visibleCountRange.BaseShaderRegister = 2;
        visibleCountRange.RegisterSpace = 1;

        visibleCountRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE
            visibilityFlagsRange{};

        visibilityFlagsRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        visibilityFlagsRange.NumDescriptors = 1;
        visibilityFlagsRange.BaseShaderRegister = 3;
        visibilityFlagsRange.RegisterSpace = 1;

        visibilityFlagsRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER parameters[6]{};

        parameters[0].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_CBV;

        parameters[0].Descriptor.ShaderRegister = 0;
        parameters[0].Descriptor.RegisterSpace = 0;

        parameters[0].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;

        parameters[1].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[1].DescriptorTable.NumDescriptorRanges =
            1;

        parameters[1].DescriptorTable.pDescriptorRanges =
            &textureRange;

        parameters[1].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;

        parameters[2].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[2]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[2]
            .DescriptorTable
            .pDescriptorRanges =
            &debrisInstancesRange;

        parameters[2].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;

        parameters[3].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[3]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[3]
            .DescriptorTable
            .pDescriptorRanges =
            &visibleInstanceIndicesRange;

        parameters[3].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;


        parameters[4].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[4]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[4]
            .DescriptorTable
            .pDescriptorRanges =
            &visibleCountRange;

        parameters[4].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;

        parameters[5].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[5]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[5]
            .DescriptorTable
            .pDescriptorRanges =
            &visibilityFlagsRange;

        parameters[5].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_STATIC_SAMPLER_DESC sampler{};

        sampler.Filter =
            D3D12_FILTER_MIN_MAG_MIP_LINEAR;

        sampler.AddressU =
            D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        sampler.AddressV =
            D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        sampler.AddressW =
            D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 1;

        sampler.ComparisonFunc =
            D3D12_COMPARISON_FUNC_ALWAYS;

        sampler.BorderColor =
            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;

        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;

        sampler.ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC description{};

        description.NumParameters =
            _countof(parameters);

        description.pParameters =
            parameters;

        description.NumStaticSamplers = 1;
        description.pStaticSamplers = &sampler;

        description.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
            "D3D12SerializeRootSignature failed"
        );

        core::ThrowIfFailed(
            device->CreateRootSignature(
                0,
                serializedSignature->GetBufferPointer(),
                serializedSignature->GetBufferSize(),
                IID_PPV_ARGS(
                    m_rootSignature
                    .ReleaseAndGetAddressOf()
                )
            ),
            "ID3D12Device::CreateRootSignature failed"
        );

        core::ThrowIfFailed(
            m_rootSignature->SetName(
                L"Gravity Forge Basic Root Signature"
            ),
            "Failed to name basic root signature"
        );
    }

    void BasicPipeline::CreatePipelineStates(
        ID3D12Device* device,
        DXGI_FORMAT renderTargetFormat)
    {
        D3D12_RENDER_TARGET_BLEND_DESC
            renderTargetBlend{};

        renderTargetBlend.BlendEnable = FALSE;
        renderTargetBlend.LogicOpEnable = FALSE;

        renderTargetBlend.SrcBlend =
            D3D12_BLEND_ONE;

        renderTargetBlend.DestBlend =
            D3D12_BLEND_ZERO;

        renderTargetBlend.BlendOp =
            D3D12_BLEND_OP_ADD;

        renderTargetBlend.SrcBlendAlpha =
            D3D12_BLEND_ONE;

        renderTargetBlend.DestBlendAlpha =
            D3D12_BLEND_ZERO;

        renderTargetBlend.BlendOpAlpha =
            D3D12_BLEND_OP_ADD;

        renderTargetBlend.LogicOp =
            D3D12_LOGIC_OP_NOOP;

        renderTargetBlend.RenderTargetWriteMask =
            D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_BLEND_DESC blendState{};

        blendState.AlphaToCoverageEnable = FALSE;
        blendState.IndependentBlendEnable = FALSE;

        blendState.RenderTarget[0] = renderTargetBlend;

        D3D12_RASTERIZER_DESC rasterizerState{};

        rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

        rasterizerState.CullMode = D3D12_CULL_MODE_BACK;

        rasterizerState.FrontCounterClockwise =
            FALSE;

        rasterizerState.DepthBias =
            D3D12_DEFAULT_DEPTH_BIAS;

        rasterizerState.DepthBiasClamp =
            D3D12_DEFAULT_DEPTH_BIAS_CLAMP;

        rasterizerState.SlopeScaledDepthBias =
            D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

        rasterizerState.DepthClipEnable = TRUE;
        rasterizerState.MultisampleEnable = FALSE;
        rasterizerState.AntialiasedLineEnable = FALSE;
        rasterizerState.ForcedSampleCount = 0;

        rasterizerState.ConservativeRaster =
            D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        D3D12_DEPTH_STENCILOP_DESC stencilFace{};

        stencilFace.StencilFailOp =
            D3D12_STENCIL_OP_KEEP;

        stencilFace.StencilDepthFailOp =
            D3D12_STENCIL_OP_KEEP;

        stencilFace.StencilPassOp =
            D3D12_STENCIL_OP_KEEP;

        stencilFace.StencilFunc =
            D3D12_COMPARISON_FUNC_ALWAYS;

        D3D12_DEPTH_STENCIL_DESC depthStencilState{};

        depthStencilState.DepthEnable = TRUE;

        depthStencilState.DepthWriteMask =
            D3D12_DEPTH_WRITE_MASK_ALL;

        depthStencilState.DepthFunc =
            D3D12_COMPARISON_FUNC_LESS;

        depthStencilState.StencilEnable = FALSE;

        depthStencilState.StencilReadMask =
            D3D12_DEFAULT_STENCIL_READ_MASK;

        depthStencilState.StencilWriteMask =
            D3D12_DEFAULT_STENCIL_WRITE_MASK;

        depthStencilState.FrontFace = stencilFace;
        depthStencilState.BackFace = stencilFace;

        const D3D12_INPUT_ELEMENT_DESC
            inputElements[]
        {
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            },
            {
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                D3D12_APPEND_ALIGNED_ELEMENT,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            },
            {
                "TEXCOORD",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                0,
                D3D12_APPEND_ALIGNED_ELEMENT,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC
            description{};

        description.pRootSignature = m_rootSignature.Get();

        description.VS.pShaderBytecode = g_BasicVertexShader;

        description.VS.BytecodeLength = sizeof(g_BasicVertexShader);

        description.PS.pShaderBytecode = g_BasicPixelShader;

        description.PS.BytecodeLength = sizeof(g_BasicPixelShader);

        description.BlendState = blendState;

        description.RasterizerState = rasterizerState;

        description.DepthStencilState = depthStencilState;

        description.InputLayout.pInputElementDescs = inputElements;

        description.InputLayout.NumElements = _countof(inputElements);

        description.SampleMask = std::numeric_limits<UINT>::max();

        description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        description.NumRenderTargets = 1;

        description.RTVFormats[0] = renderTargetFormat;

        description.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        const D3D12_SHADER_BYTECODE
            surfacePixelShader{
                g_BasicPixelShader,
                sizeof(g_BasicPixelShader)
        };

        const D3D12_SHADER_BYTECODE
            emissivePixelShader{
                g_EmissivePixelShader,
                sizeof(g_EmissivePixelShader)
        };

        const D3D12_SHADER_BYTECODE
            debrisPixelShader{
                g_DebrisPixelShader,
                sizeof(g_DebrisPixelShader)
        };

        const auto createPipelineState =
            [
                device,
                &description
            ](
                const D3D12_SHADER_BYTECODE&
                pixelShader,
                bool depthEnabled,
                Microsoft::WRL::ComPtr<
                ID3D12PipelineState
                >& output,
                const wchar_t* debugName)
            {
                description.PS =
                    pixelShader;

                description.DepthStencilState.DepthEnable =
                    depthEnabled ? TRUE : FALSE;

                description.DepthStencilState.DepthWriteMask =
                    depthEnabled
                    ? D3D12_DEPTH_WRITE_MASK_ALL
                    : D3D12_DEPTH_WRITE_MASK_ZERO;

                core::ThrowIfFailed(
                    device->CreateGraphicsPipelineState(
                        &description,
                        IID_PPV_ARGS(
                            output.ReleaseAndGetAddressOf()
                        )
                    ),
                    "Failed to create graphics pipeline state"
                );

                core::ThrowIfFailed(
                    output->SetName(debugName),
                    "Failed to name graphics pipeline state"
                );
            };

        createPipelineState(
            surfacePixelShader,
            true,
            m_depthEnabledPipelineState,
            L"Gravity Forge Surface Pipeline "
            L"Depth Enabled"
        );

        createPipelineState(
            surfacePixelShader,
            false,
            m_depthDisabledPipelineState,
            L"Gravity Forge Surface Pipeline "
            L"Depth Disabled"
        );

        createPipelineState(
            emissivePixelShader,
            true,
            m_emissiveDepthEnabledPipelineState,
            L"Gravity Forge Emissive Pipeline "
            L"Depth Enabled"
        );

        createPipelineState(
            emissivePixelShader,
            false,
            m_emissiveDepthDisabledPipelineState,
            L"Gravity Forge Emissive Pipeline "
            L"Depth Disabled"
        );

        description.VS = {
            g_DebrisVertexShader,
            sizeof(g_DebrisVertexShader)
        };

        createPipelineState(
            debrisPixelShader,
            true,
            m_debrisDepthEnabledPipelineState,
            L"Gravity Forge Debris Pipeline Depth Enabled"
        );

        createPipelineState(
            debrisPixelShader,
            false,
            m_debrisDepthDisabledPipelineState,
            L"Gravity Forge Debris Pipeline Depth Disabled"
        );
    }
}
