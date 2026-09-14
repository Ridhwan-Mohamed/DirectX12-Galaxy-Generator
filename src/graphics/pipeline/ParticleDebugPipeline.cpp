#include "ParticleDebugPipeline.h"

#include "../../core/HResult.h"

#include "ParticleDebugVS.h"
#include "ParticleDebugPS.h"

#include <Windows.h>

#include <limits>
#include <stdexcept>

namespace gf::graphics::pipeline
{
    ParticleDebugPipeline::
        ParticleDebugPipeline(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat,
            DXGI_FORMAT depthFormat)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "ParticleDebugPipeline requires "
                "a valid D3D12 device"
            );
        }

        if (
            renderTargetFormat ==
            DXGI_FORMAT_UNKNOWN ||
            depthFormat ==
            DXGI_FORMAT_UNKNOWN)
        {
            throw std::invalid_argument(
                "ParticleDebugPipeline requires "
                "valid target formats"
            );
        }

        CreateRootSignature(
            device
        );

        CreatePipelineState(
            device,
            renderTargetFormat,
            depthFormat
        );
    }

    void ParticleDebugPipeline::
        CreateRootSignature(
            ID3D12Device* device)
    {
        D3D12_DESCRIPTOR_RANGE
            particleStateRange{};

        particleStateRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        particleStateRange.NumDescriptors = 1;

        particleStateRange.BaseShaderRegister = 0;
        particleStateRange.RegisterSpace = 0;

        particleStateRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


        D3D12_ROOT_PARAMETER
            parameters[2]{};


        // b0:
        // camera transform + debug color.
        parameters[0].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_CBV;

        parameters[0]
            .Descriptor
            .ShaderRegister = 0;

        parameters[0]
            .Descriptor
            .RegisterSpace = 0;

        parameters[0].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;


        // t0:
        // current particle-state SRV.
        parameters[1].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[1]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[1]
            .DescriptorTable
            .pDescriptorRanges =
            &particleStateRange;

        parameters[1].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;


        D3D12_ROOT_SIGNATURE_DESC
            description{};

        description.NumParameters =
            _countof(parameters);

        description.pParameters =
            parameters;

        description.NumStaticSamplers = 0;
        description.pStaticSamplers = nullptr;

        description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
            "debug root signature"
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
            "debug root signature"
        );


        core::ThrowIfFailed(
            m_rootSignature->SetName(
                L"Gravity Forge "
                L"Particle Debug Root Signature"
            ),
            "Failed to name particle "
            "debug root signature"
        );
    }

    void ParticleDebugPipeline::
        CreatePipelineState(
            ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat,
            DXGI_FORMAT depthFormat)
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


        D3D12_BLEND_DESC
            blendState{};

        blendState.AlphaToCoverageEnable = FALSE;
        blendState.IndependentBlendEnable = FALSE;

        blendState.RenderTarget[0] =
            renderTargetBlend;


        D3D12_RASTERIZER_DESC
            rasterizerState{};

        rasterizerState.FillMode =
            D3D12_FILL_MODE_SOLID;

        rasterizerState.CullMode =
            D3D12_CULL_MODE_NONE;

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


        D3D12_DEPTH_STENCILOP_DESC
            stencilFace{};

        stencilFace.StencilFailOp =
            D3D12_STENCIL_OP_KEEP;

        stencilFace.StencilDepthFailOp =
            D3D12_STENCIL_OP_KEEP;

        stencilFace.StencilPassOp =
            D3D12_STENCIL_OP_KEEP;

        stencilFace.StencilFunc =
            D3D12_COMPARISON_FUNC_ALWAYS;


        D3D12_DEPTH_STENCIL_DESC
            depthStencilState{};

        depthStencilState.DepthEnable = TRUE;

        depthStencilState.DepthWriteMask =
            D3D12_DEPTH_WRITE_MASK_ZERO;

        depthStencilState.DepthFunc =
            D3D12_COMPARISON_FUNC_LESS_EQUAL;

        depthStencilState.StencilEnable = FALSE;

        depthStencilState.StencilReadMask =
            D3D12_DEFAULT_STENCIL_READ_MASK;

        depthStencilState.StencilWriteMask =
            D3D12_DEFAULT_STENCIL_WRITE_MASK;

        depthStencilState.FrontFace =
            stencilFace;

        depthStencilState.BackFace =
            stencilFace;


        D3D12_GRAPHICS_PIPELINE_STATE_DESC
            description{};

        description.pRootSignature =
            m_rootSignature.Get();


        description.VS.pShaderBytecode =
            g_ParticleDebugVertexShader;

        description.VS.BytecodeLength =
            sizeof(
                g_ParticleDebugVertexShader
                );


        description.PS.pShaderBytecode =
            g_ParticleDebugPixelShader;

        description.PS.BytecodeLength =
            sizeof(
                g_ParticleDebugPixelShader
                );


        // No vertex-buffer attributes.
        description.InputLayout =
            D3D12_INPUT_LAYOUT_DESC{
                nullptr,
                0
        };


        description.BlendState =
            blendState;

        description.SampleMask =
            std::numeric_limits<UINT>::max();

        description.RasterizerState =
            rasterizerState;

        description.DepthStencilState =
            depthStencilState;


        description.IBStripCutValue =
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

        description.PrimitiveTopologyType =
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;


        description.NumRenderTargets = 1;

        description.RTVFormats[0] =
            renderTargetFormat;

        description.DSVFormat =
            depthFormat;


        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.NodeMask = 0;

        description.Flags =
            D3D12_PIPELINE_STATE_FLAG_NONE;


        core::ThrowIfFailed(
            device->CreateGraphicsPipelineState(
                &description,
                IID_PPV_ARGS(
                    m_pipelineState
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create particle "
            "debug pipeline"
        );


        core::ThrowIfFailed(
            m_pipelineState->SetName(
                L"Gravity Forge "
                L"Particle Debug Point Pipeline"
            ),
            "Failed to name particle "
            "debug pipeline"
        );
    }
}