#include "PresentationPipeline.h"

#include "../../core/HResult.h"

#include "HdrPresentPS.h"
#include "HdrPresentVS.h"

#include <Windows.h>

#include <limits>
#include <stdexcept>

namespace gf::graphics::pipeline
{
    PresentationPipeline::PresentationPipeline(
        ID3D12Device* device,
        DXGI_FORMAT renderTargetFormat)
    {
        if (device == nullptr)
        {
            throw std::invalid_argument(
                "PresentationPipeline requires "
                "a valid D3D12 device"
            );
        }

        if (renderTargetFormat ==
            DXGI_FORMAT_UNKNOWN)
        {
            throw std::invalid_argument(
                "PresentationPipeline requires "
                "a valid render-target format"
            );
        }

        CreateRootSignature(device);

        CreatePipelineState(
            device,
            renderTargetFormat
        );
    }

    void PresentationPipeline::CreateRootSignature(
        ID3D12Device* device)
    {
        D3D12_DESCRIPTOR_RANGE
            hdrTextureRange{};

        hdrTextureRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        hdrTextureRange.NumDescriptors = 1;

        hdrTextureRange.BaseShaderRegister = 0;
        hdrTextureRange.RegisterSpace = 0;

        hdrTextureRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE
            bloomTextureRange{};

        bloomTextureRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        bloomTextureRange.NumDescriptors = 1;

        bloomTextureRange.BaseShaderRegister = 1;

        bloomTextureRange.RegisterSpace = 0;

        bloomTextureRange
            .OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER
            parameters[3]{};

        parameters[0].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[0]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[0]
            .DescriptorTable
            .pDescriptorRanges =
            &hdrTextureRange;

        parameters[0].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;

        parameters[1].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        parameters[1]
            .DescriptorTable
            .NumDescriptorRanges = 1;

        parameters[1]
            .DescriptorTable
            .pDescriptorRanges =
            &bloomTextureRange;

        parameters[1].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;

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
            .Num32BitValues = 2;

        parameters[2].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC
            sampler{};

        sampler.Filter =
            D3D12_FILTER_MIN_MAG_MIP_LINEAR;

        sampler.AddressU =
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        sampler.AddressV =
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        sampler.AddressW =
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        sampler.MipLODBias = 0.0f;

        sampler.MaxAnisotropy = 1;

        sampler.ComparisonFunc =
            D3D12_COMPARISON_FUNC_ALWAYS;

        sampler.BorderColor =
            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

        sampler.MinLOD = 0.0f;

        sampler.MaxLOD =
            D3D12_FLOAT32_MAX;

        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;

        sampler.ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC
            description{};

        description.NumParameters =
            _countof(parameters);

        description.pParameters =
            parameters;

        description.NumStaticSamplers = 1;

        description.pStaticSamplers =
            &sampler;

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
            "Failed to serialize "
            "presentation root signature"
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
            "Failed to create "
            "presentation root signature"
        );

        core::ThrowIfFailed(
            m_rootSignature->SetName(
                L"Gravity Forge "
                L"Presentation Root Signature"
            ),
            "Failed to name "
            "presentation root signature"
        );
    }

    void PresentationPipeline::CreatePipelineState(
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

        blendState.AlphaToCoverageEnable =
            FALSE;

        blendState.IndependentBlendEnable =
            FALSE;

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

        rasterizerState.AntialiasedLineEnable =
            FALSE;

        rasterizerState.ForcedSampleCount = 0;

        rasterizerState.ConservativeRaster =
            D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        D3D12_DEPTH_STENCIL_DESC
            depthStencilState{};

        depthStencilState.DepthEnable = FALSE;

        depthStencilState.DepthWriteMask =
            D3D12_DEPTH_WRITE_MASK_ZERO;

        depthStencilState.DepthFunc =
            D3D12_COMPARISON_FUNC_ALWAYS;

        depthStencilState.StencilEnable = FALSE;

        depthStencilState.StencilReadMask =
            D3D12_DEFAULT_STENCIL_READ_MASK;

        depthStencilState.StencilWriteMask =
            D3D12_DEFAULT_STENCIL_WRITE_MASK;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC
            description{};

        description.pRootSignature =
            m_rootSignature.Get();

        description.VS.pShaderBytecode =
            g_HdrPresentVertexShader;

        description.VS.BytecodeLength =
            sizeof(g_HdrPresentVertexShader);

        description.PS.pShaderBytecode =
            g_HdrPresentPixelShader;

        description.PS.BytecodeLength =
            sizeof(g_HdrPresentPixelShader);

        description.BlendState =
            blendState;

        description.RasterizerState =
            rasterizerState;

        description.DepthStencilState =
            depthStencilState;

        description.InputLayout
            .pInputElementDescs = nullptr;

        description.InputLayout
            .NumElements = 0;

        description.SampleMask =
            std::numeric_limits<UINT>::max();

        description.PrimitiveTopologyType =
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        description.NumRenderTargets = 1;

        description.RTVFormats[0] =
            renderTargetFormat;

        description.DSVFormat =
            DXGI_FORMAT_UNKNOWN;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        core::ThrowIfFailed(
            device->CreateGraphicsPipelineState(
                &description,
                IID_PPV_ARGS(
                    m_pipelineState
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create "
            "presentation pipeline"
        );

        core::ThrowIfFailed(
            m_pipelineState->SetName(
                L"Gravity Forge "
                L"HDR Presentation Pipeline"
            ),
            "Failed to name "
            "presentation pipeline"
        );
    }
}