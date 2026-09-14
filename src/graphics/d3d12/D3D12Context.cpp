#include "D3D12Context.h"

#include "../../core/HResult.h"
#include "../../core/Log.h"
#include "../../core/Win32Error.h"
#include "../pipeline/BasicPipeline.h"
#include "../mesh/Mesh.h"
#include "../../scene/Camera.h"
#include "../texture/Rgba8Image.h"
#include "../texture/Texture2D.h"
#include "../texture/WicImageLoader.h"
#include "../mesh/PrimitiveMeshes.h"
#include "../pipeline/PresentationPipeline.h"
#include "../../simulation/ParticleSystem.h"
#include "../pipeline/ParticleSimulationPipeline.h"
#include "../pipeline/ParticleDebugPipeline.h"
#include "../pipeline/ParticleBillboardPipeline.h"
#include "../pipeline/DebrisCullingPipeline.h"
#include "../pipeline/BloomExtractPipeline.h"
#include "../pipeline/BloomBlurPipeline.h"
#include "../../scene/DebrisField.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <utility>
#include <cmath>
#include <random>
#include <pix3.h>
#include <cstring>

namespace
{

    struct GravitySourceRootData
    {
        float positionX;
        float positionY;
        float positionZ;
        float strength;
    };

    static_assert(
        sizeof(GravitySourceRootData) ==
        16
    );

    struct ParticleSimulationRootConstants
    {
        //
        // DWORD 0 - 3
        //
        float deltaTime;
        float simulationPadding0;
        float softeningSquared;
        std::uint32_t particleCount;


        //
        // DWORD 4 - 15
        //
        // Three sources x four DWORDs.
        //
        std::array<
            GravitySourceRootData,
            gf::graphics::d3d12::
            MaxInteractiveGravitySources
        > interactiveGravitySources;


        //
        // DWORD 16 - 19
        //
        std::uint32_t
            interactiveGravitySourceCount;

        std::uint32_t
            simulationMode;

        float explosionStrength;

        float explosionTangentialStrength;

        float reformPositionStrength;

        float reformVelocityStrength;

        float reformMaxAcceleration;

        float reformPadding;

        //
        // Galaxy topology.
        // DWORD 24 - 39.
        //
        std::uint32_t galaxyArmCount;
        float galaxyArmPitch;
        float galaxyArmConcentration;
        float galaxyInterArmFraction;

        float galaxySpiralSpread;
        float galaxyCoreFalloff;
        float galaxySpin;
        float galaxyInnerArmRadius;

        float galaxyOuterArmRadius;
        float galaxyCoreRadius;
        float galaxyCoreParticleFraction;
        float galaxyCoreHalfThickness;

        float galaxyDiskHalfThickness;
        float galaxyRotationCurveRadius;
        float galaxyDifferentialRotationStrength;
        float galaxyPadding0;

        //
        // Galaxy perturbation controls.
        // DWORD 40 - 43.
        //
        float galaxyRadialPerturbationStrength;
        float galaxyRadialPerturbationFrequencyScale;
        float galaxyRadialVelocityDamping;
        float galaxyVerticalOscillationFrequency;

        float galaxyRegularArmHalfWidthInnerDegrees;
        float galaxyRegularArmHalfWidthOuterDegrees;
        float galaxyRegularArmHalfWidthRadiusPower;
        float galaxyRegularArmHalfWidthOuterBias;

        float galaxyInterArmFractionInner;
        float galaxyInterArmFractionOuter;
        float galaxyInterArmFractionRadiusPower;
        float galaxyInterArmFractionOuterBias;
    };

    static_assert(
        sizeof(ParticleSimulationRootConstants)
        == 208
        );

    constexpr float
        ParticleSofteningRadius =
        0.35f;

    constexpr float
        MaximumParticleDeltaTime =
        1.0f / 30.0f;

    constexpr UINT
        ParticleThreadsPerGroup =
        256;

    constexpr UINT
        ParticleBillboardVertexCount =
        6;

    constexpr float
        ParticleBillboardHalfSize =
        0.015f;

    constexpr std::uint32_t
        DebrisFrustumPlaneCount = 6;

    constexpr UINT
        DebrisCullingThreadsPerGroup = 64;

    struct DebrisCullingRootConstants
    {
        std::array<
            DirectX::XMFLOAT4,
            DebrisFrustumPlaneCount
        > frustumPlanes;

        float animationSeconds;
        std::uint32_t instanceCount;
        float frustumInset;
        float padding;
    };

    static_assert(
        sizeof(DebrisCullingRootConstants) == 
        112
        );

    using GalaxyColor =
        std::array<float, 3>;

    std::mt19937& GalaxyColorRng()
    {
        static std::mt19937 randomEngine{
            std::random_device{}()
        };

        return randomEngine;
    }

    const std::array<GalaxyColor, 10>
        GalaxyWarmColorPresets{
            GalaxyColor{1.00f, 0.56f, 0.32f},
            GalaxyColor{1.00f, 0.70f, 0.35f},
            GalaxyColor{1.00f, 0.81f, 0.50f},
            GalaxyColor{1.00f, 0.65f, 0.40f},
            GalaxyColor{0.98f, 0.74f, 0.34f},
            GalaxyColor{1.00f, 0.54f, 0.60f},
            GalaxyColor{1.00f, 0.43f, 0.24f},
            GalaxyColor{0.96f, 0.76f, 0.42f},
            GalaxyColor{1.00f, 0.68f, 0.30f},
            GalaxyColor{0.95f, 0.57f, 0.21f}
        };

    const std::array<GalaxyColor, 10>
        GalaxyCoolColorPresets{
            GalaxyColor{0.40f, 0.60f, 1.00f},
            GalaxyColor{0.36f, 0.66f, 1.00f},
            GalaxyColor{0.48f, 0.72f, 1.00f},
            GalaxyColor{0.38f, 0.62f, 0.96f},
            GalaxyColor{0.52f, 0.78f, 0.98f},
            GalaxyColor{0.32f, 0.63f, 0.92f},
            GalaxyColor{0.44f, 0.69f, 0.86f},
            GalaxyColor{0.50f, 0.70f, 0.95f},
            GalaxyColor{0.30f, 0.55f, 0.88f},
            GalaxyColor{0.56f, 0.82f, 0.97f}
        };

    const std::array<GalaxyColor, 10>
        GalaxyWackyColorPresets{
            GalaxyColor{1.00f, 0.20f, 0.75f},
            GalaxyColor{1.00f, 0.35f, 0.10f},
            GalaxyColor{0.95f, 0.15f, 1.00f},
            GalaxyColor{1.00f, 0.92f, 0.20f},
            GalaxyColor{0.12f, 1.00f, 0.30f},
            GalaxyColor{0.00f, 0.95f, 0.95f},
            GalaxyColor{0.95f, 0.00f, 0.50f},
            GalaxyColor{0.00f, 0.65f, 1.00f},
            GalaxyColor{1.00f, 0.60f, 0.00f},
            GalaxyColor{0.85f, 0.00f, 0.85f}
        };

    DirectX::XMFLOAT4 NormalizePlane(
        const DirectX::XMFLOAT4& plane)
    {
        const float normalLength =
            std::sqrt(
                plane.x * plane.x +
                plane.y * plane.y +
                plane.z * plane.z
            );

        if (normalLength <= 0.000001f)
        {
            throw std::runtime_error(
                "Cannot normalize degenerate "
                "frustum plane"
            );
        }

        const float inverseLength =
            1.0f / normalLength;

        return DirectX::XMFLOAT4{
            plane.x * inverseLength,
            plane.y * inverseLength,
            plane.z * inverseLength,
            plane.w * inverseLength
        };
    }

    std::array<
        DirectX::XMFLOAT4,
        DebrisFrustumPlaneCount
    > ExtractFrustumPlanes(
        const DirectX::XMMATRIX&
        viewProjection)
    {
        DirectX::XMFLOAT4X4 matrix{};

        DirectX::XMStoreFloat4x4(
            &matrix,
            viewProjection
        );

        std::array<
            DirectX::XMFLOAT4,
            DebrisFrustumPlaneCount
        > planes{};

        // DirectX clip-space conditions:
        //
        // -w <= x <= w
        // -w <= y <= w
        //  0 <= z <= w
        //
        // This project uses row-vector multiplication:
        // mul(worldPosition, viewProjection).
        //
        // The planes are therefore built from matrix columns.

        // Left: x + w >= 0.
        planes[0] = NormalizePlane({
            matrix._11 + matrix._14,
            matrix._21 + matrix._24,
            matrix._31 + matrix._34,
            matrix._41 + matrix._44
            });

        // Right: w - x >= 0.
        planes[1] = NormalizePlane({
            matrix._14 - matrix._11,
            matrix._24 - matrix._21,
            matrix._34 - matrix._31,
            matrix._44 - matrix._41
            });

        // Bottom: y + w >= 0.
        planes[2] = NormalizePlane({
            matrix._12 + matrix._14,
            matrix._22 + matrix._24,
            matrix._32 + matrix._34,
            matrix._42 + matrix._44
            });

        // Top: w - y >= 0.
        planes[3] = NormalizePlane({
            matrix._14 - matrix._12,
            matrix._24 - matrix._22,
            matrix._34 - matrix._32,
            matrix._44 - matrix._42
            });

        // Near: z >= 0.
        planes[4] = NormalizePlane({
            matrix._13,
            matrix._23,
            matrix._33,
            matrix._43
            });

        // Far: w - z >= 0.
        planes[5] = NormalizePlane({
            matrix._14 - matrix._13,
            matrix._24 - matrix._23,
            matrix._34 - matrix._33,
            matrix._44 - matrix._43
            });

        return planes;
    }
}

namespace gf::graphics::d3d12
{
    D3D12Context::D3D12Context(
        HWND windowHandle,
        std::uint32_t width,
        std::uint32_t height)
        : m_windowHandle(windowHandle),
        m_width(width),
        m_height(height)
    {
        if (windowHandle == nullptr)
        {
            throw std::invalid_argument(
                "D3D12Context requires a valid window handle"
            );
        }

        if (width == 0 || height == 0)
        {
            throw std::invalid_argument(
                "D3D12Context requires nonzero dimensions"
            );
        }

        EnableDebugLayer();
        CreateFactory();
        SelectAdapter();
        CreateDevice();

        CreateCommandQueue();
        CreateSwapChain();
        CreateRtvHeap();
        CreateBackBuffers();

        CreateDsvHeap();
        CreateDepthBuffer();

        CreatePipeline();
        CreateSrvHeap();

        UpdateViewportAndScissor();

        CreateCommandObjects();
        CreateSynchronizationObjects();
        CreateReactorMeshes();
        CreateTestTexture();
        CreateTestTextureSrvs();

        m_hdrSceneSrv = AllocateSrv();
        CreateHdrSceneTarget();

        m_bloomExtractSrv = AllocateSrv();
        CreateBloomExtractTarget();

        m_bloomBlurTempSrv = AllocateSrv();
        CreateBloomBlurTempTarget();

        CreateParticleSystem();
        CreateParticleBufferViews();

        core::LogInfo("D3D12 presentation foundation created");
    }

    D3D12Context::~D3D12Context() noexcept
    {
        try
        {
            FlushGpu();
        }
        catch (const std::exception& error)
        {
            OutputDebugStringA(
                "D3D12 shutdown error: "
            );

            OutputDebugStringA(error.what());
            OutputDebugStringA("\n");
        }

        if (m_fenceEvent != nullptr)
        {
            if (!CloseHandle(m_fenceEvent))
            {
                OutputDebugStringA(
                    "Failed to close D3D12 fence event\n"
                );
            }

            m_fenceEvent = nullptr;
        }
    }

    void D3D12Context::EnableDebugLayer()
    {
        #if defined(_DEBUG)
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

            core::ThrowIfFailed(
                D3D12GetDebugInterface(
                    IID_PPV_ARGS(
                        debugController.ReleaseAndGetAddressOf()
                    )
                ),
                "D3D12GetDebugInterface failed"
            );

            debugController->EnableDebugLayer();
            m_factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

            core::LogInfo("D3D12 debug layer enabled");
        #endif
    }

    void D3D12Context::CreateFactory()
    {
        core::ThrowIfFailed(
            CreateDXGIFactory2(
                m_factoryFlags,
                IID_PPV_ARGS(
                    m_factory.ReleaseAndGetAddressOf()
                )
            ),
            "CreateDXGIFactory2 failed"
        );

        core::LogInfo("DXGI factory created");
    }

    void D3D12Context::SelectAdapter()
    {
        for (UINT adapterIndex = 0; ; ++adapterIndex)
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter1> candidate;

            const HRESULT result =
                m_factory->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(
                        candidate.ReleaseAndGetAddressOf()
                    )
                );

            if (result == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }

            core::ThrowIfFailed(
                result,
                "EnumAdapterByGpuPreference failed"
            );

            DXGI_ADAPTER_DESC1 description{};

            core::ThrowIfFailed(
                candidate->GetDesc1(&description),
                "IDXGIAdapter1::GetDesc1 failed"
            );

            if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
            {
                continue;
            }

            const HRESULT deviceProbe = D3D12CreateDevice(
                candidate.Get(),
                D3D_FEATURE_LEVEL_12_0,
                __uuidof(ID3D12Device),
                nullptr
            );

            if (SUCCEEDED(deviceProbe))
            {
                m_adapter = std::move(candidate);
                break;
            }
        }

        if (m_adapter == nullptr)
        {
            throw std::runtime_error(
                "No hardware adapter supports D3D feature level 12_0"
            );
        }

        core::LogInfo("Compatible hardware adapter selected");
    }

    void D3D12Context::CreateDevice()
    {
        core::ThrowIfFailed(
            D3D12CreateDevice(
                m_adapter.Get(),
                D3D_FEATURE_LEVEL_12_0,
                IID_PPV_ARGS(
                    m_device.ReleaseAndGetAddressOf()
                )
            ),
            "D3D12CreateDevice failed"
        );

        core::ThrowIfFailed(
            m_device->SetName(
                L"Gravity Forge D3D12 Device"
            ),
            "Failed to name D3D12 device"
        );

        core::LogInfo("D3D12 device created");
    }

    void D3D12Context::CreateCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC description{};

        description.Type =
            D3D12_COMMAND_LIST_TYPE_DIRECT;

        description.Priority =
            D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        description.Flags =
            D3D12_COMMAND_QUEUE_FLAG_NONE;

        description.NodeMask = 0;

        core::ThrowIfFailed(
            m_device->CreateCommandQueue(
                &description,
                IID_PPV_ARGS(
                    m_commandQueue.ReleaseAndGetAddressOf()
                )
            ),
            "ID3D12Device::CreateCommandQueue failed"
        );

        core::ThrowIfFailed(
            m_commandQueue->SetName(
                L"Gravity Forge Direct Command Queue"
            ),
            "Failed to name direct command queue"
        );

        core::LogInfo("Direct command queue created");
    }

    void D3D12Context::CreateSwapChain()
    {
        DXGI_SWAP_CHAIN_DESC1 description{};

        description.Width = m_width;
        description.Height = m_height;

        description.Format = BackBufferFormat;

        description.Stereo = FALSE;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT;

        description.BufferCount = SwapChainBufferCount;

        description.Scaling =
            DXGI_SCALING_STRETCH;

        description.SwapEffect =
            DXGI_SWAP_EFFECT_FLIP_DISCARD;

        description.AlphaMode =
            DXGI_ALPHA_MODE_UNSPECIFIED;

        description.Flags = 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain1>
            initialSwapChain;
        core::ThrowIfFailed(
            m_factory->CreateSwapChainForHwnd(
                m_commandQueue.Get(),
                m_windowHandle,
                &description,
                nullptr,
                nullptr,
                initialSwapChain.ReleaseAndGetAddressOf()
            ),
            "IDXGIFactory::CreateSwapChainForHwnd failed"
        );

        core::ThrowIfFailed(
            m_factory->MakeWindowAssociation(
                m_windowHandle,
                DXGI_MWA_NO_ALT_ENTER
            ),
            "IDXGIFactory::MakeWindowAssociation failed"
        );

        core::ThrowIfFailed(
            initialSwapChain.As(&m_swapChain),
            "Failed to query IDXGISwapChain4"
        );

        m_backBufferIndex =
            m_swapChain->GetCurrentBackBufferIndex();

        core::LogInfo("DXGI swap chain created");
    }

    void D3D12Context::CreateRtvHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC description{};

        description.Type =
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        description.NumDescriptors = RtvHeapCapacity;

        description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        description.NodeMask = 0;

        core::ThrowIfFailed(
            m_device->CreateDescriptorHeap(
                &description,
                IID_PPV_ARGS(
                    m_rtvHeap.ReleaseAndGetAddressOf()
                )
            ),
            "ID3D12Device::CreateDescriptorHeap failed"
        );

        core::ThrowIfFailed(
            m_rtvHeap->SetName(
                L"Gravity Forge RTV Descriptor Heap"
            ),
            "Failed to name RTV descriptor heap"
        );

        m_rtvDescriptorSize =
            m_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            );

        m_hdrSceneRtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        m_hdrSceneRtvHandle.ptr +=
            static_cast<SIZE_T>(HdrSceneRtvIndex) *
            static_cast<SIZE_T>(m_rtvDescriptorSize);

        m_bloomExtractRtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        m_bloomExtractRtvHandle.ptr +=
            static_cast<SIZE_T>(BloomExtractRtvIndex) *
            static_cast<SIZE_T>(m_rtvDescriptorSize);

        m_bloomBlurTempRtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        m_bloomBlurTempRtvHandle.ptr +=
            static_cast<SIZE_T>(BloomBlurTempRtvIndex) *
            static_cast<SIZE_T>(m_rtvDescriptorSize);

        core::LogInfo("RTV descriptor heap created");
    }

    void D3D12Context::CreateSrvHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC description{};

        description.Type =
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        description.NumDescriptors =
            SrvHeapCapacity;

        description.Flags =
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        description.NodeMask = 0;

        core::ThrowIfFailed(
            m_device->CreateDescriptorHeap(
                &description,
                IID_PPV_ARGS(
                    m_srvHeap.ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create shader-visible SRV heap"
        );

        m_srvDescriptorSize =
            m_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            );

        core::ThrowIfFailed(
            m_srvHeap->SetName(
                L"Gravity Forge Shader Resource Heap"
            ),
            "Failed to name SRV descriptor heap"
        );
    }

    D3D12Context::SrvAllocation
        D3D12Context::AllocateSrv()
    {
        if (m_nextSrvDescriptor >= SrvHeapCapacity)
        {
            throw std::runtime_error(
                "Shader resource descriptor heap is full"
            );
        }

        SrvAllocation allocation{};

        allocation.index =
            m_nextSrvDescriptor;

        allocation.cpu =
            m_srvHeap
            ->GetCPUDescriptorHandleForHeapStart();

        allocation.gpu =
            m_srvHeap
            ->GetGPUDescriptorHandleForHeapStart();

        allocation.cpu.ptr +=
            static_cast<SIZE_T>(allocation.index)
            * static_cast<SIZE_T>(m_srvDescriptorSize);

        allocation.gpu.ptr +=
            static_cast<UINT64>(allocation.index)
            * static_cast<UINT64>(m_srvDescriptorSize);

        ++m_nextSrvDescriptor;

        return allocation;
    }

    void D3D12Context::CreateBackBuffers()
    {
        D3D12_CPU_DESCRIPTOR_HANDLE currentHandle =
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (UINT index = 0; index < SwapChainBufferCount; ++index)
        {
            core::ThrowIfFailed(
                m_swapChain->GetBuffer(
                    index,
                    IID_PPV_ARGS(m_backBuffers[index].ReleaseAndGetAddressOf())
                ),
                "IDXGISwapChain::GetBuffer failed"
            );

            const std::wstring bufferName =
                std::format(
                    L"Gravity Forge Back Buffer {}",
                    index
                );

            core::ThrowIfFailed(
                m_backBuffers[index]->SetName(
                    bufferName.c_str()
                ),
                "Failed to name swap-chain back buffer"
            );

            m_rtvHandles[index] = currentHandle;

            m_device->CreateRenderTargetView(
                m_backBuffers[index].Get(),
                nullptr,
                currentHandle
            );

            currentHandle.ptr +=
                static_cast<SIZE_T>(
                    m_rtvDescriptorSize
                    );
        }

        core::LogInfo(
            "Swap-chain back buffers and RTVs created"
        );
    }

    void D3D12Context::CreateDsvHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC
            description{};

        description.Type =
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        description.NumDescriptors = 1;

        description.Flags =
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        description.NodeMask = 0;

        core::ThrowIfFailed(
            m_device->CreateDescriptorHeap(
                &description,
                IID_PPV_ARGS(
                    m_dsvHeap
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create DSV heap"
        );

        core::ThrowIfFailed(
            m_dsvHeap->SetName(
                L"Gravity Forge DSV Heap"
            ),
            "Failed to name DSV heap"
        );

        m_dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        core::LogInfo(
            "DSV descriptor heap created"
        );
    }

    void D3D12Context::CreateDepthBuffer()
    {
        D3D12_HEAP_PROPERTIES
            heapProperties{};

        heapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        description.Alignment = 0;

        description.Width = m_width;
        description.Height = m_height;

        description.DepthOrArraySize = 1;
        description.MipLevels = 1;

        description.Format = DepthFormat;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_UNKNOWN;

        description.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue{};

        clearValue.Format = DepthFormat;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValue,
                IID_PPV_ARGS(
                    m_depthBuffer
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create depth buffer"
        );

        core::ThrowIfFailed(
            m_depthBuffer->SetName(
                L"Gravity Forge Depth Buffer"
            ),
            "Failed to name depth buffer"
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC
            dsvDescription{};

        dsvDescription.Format = DepthFormat;

        dsvDescription.ViewDimension =
            D3D12_DSV_DIMENSION_TEXTURE2D;

        dsvDescription.Flags =
            D3D12_DSV_FLAG_NONE;

        dsvDescription.Texture2D.MipSlice = 0;

        m_device->CreateDepthStencilView(
            m_depthBuffer.Get(),
            &dsvDescription,
            m_dsvHandle
        );

        core::LogInfo(
            "Depth buffer and DSV created"
        );
    }

    void D3D12Context::CreateHdrSceneTarget()
    {
        if (m_width == 0 ||
            m_height == 0)
        {
            throw std::runtime_error(
                "Cannot create HDR scene target "
                "with zero dimensions"
            );
        }

        D3D12_HEAP_PROPERTIES
            heapProperties{};

        heapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC
            description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        description.Alignment = 0;

        description.Width =
            m_width;

        description.Height =
            m_height;

        description.DepthOrArraySize = 1;
        description.MipLevels = 1;

        description.Format =
            HdrSceneFormat;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_UNKNOWN;

        description.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE
            clearValue{};

        clearValue.Format =
            HdrSceneFormat;

        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(
                    m_hdrSceneColor
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create HDR scene target"
        );

        core::ThrowIfFailed(
            m_hdrSceneColor->SetName(
                L"Gravity Forge HDR Scene Color"
            ),
            "Failed to name HDR scene target"
        );

        D3D12_RENDER_TARGET_VIEW_DESC
            rtvDescription{};

        rtvDescription.Format =
            HdrSceneFormat;

        rtvDescription.ViewDimension =
            D3D12_RTV_DIMENSION_TEXTURE2D;

        rtvDescription.Texture2D.MipSlice = 0;
        rtvDescription.Texture2D.PlaneSlice = 0;

        m_device->CreateRenderTargetView(
            m_hdrSceneColor.Get(),
            &rtvDescription,
            m_hdrSceneRtvHandle
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC
            srvDescription{};

        srvDescription.Format =
            HdrSceneFormat;

        srvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_TEXTURE2D;

        srvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDescription.Texture2D.MostDetailedMip = 0;
        srvDescription.Texture2D.MipLevels = 1;
        srvDescription.Texture2D.PlaneSlice = 0;
        srvDescription.Texture2D.ResourceMinLODClamp =
            0.0f;

        m_device->CreateShaderResourceView(
            m_hdrSceneColor.Get(),
            &srvDescription,
            m_hdrSceneSrv.cpu
        );

        core::LogInfo(
            "HDR scene target, RTV, and SRV created"
        );
    }

    void D3D12Context::CreateBloomExtractTarget()
    {
        if (
            m_width == 0 ||
            m_height == 0)
        {
            throw std::runtime_error(
                "Cannot create bloom extraction target "
                "with zero dimensions"
            );
        }


        const UINT bloomWidth =
            std::max(
                1u,
                m_width / 2u
            );

        const UINT bloomHeight =
            std::max(
                1u,
                m_height / 2u
            );


        D3D12_HEAP_PROPERTIES
            heapProperties{};

        heapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;


        D3D12_RESOURCE_DESC
            description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        description.Alignment = 0;

        description.Width =
            bloomWidth;

        description.Height =
            bloomHeight;

        description.DepthOrArraySize = 1;
        description.MipLevels = 1;

        description.Format =
            HdrSceneFormat;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_UNKNOWN;

        description.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;


        D3D12_CLEAR_VALUE
            clearValue{};

        clearValue.Format =
            HdrSceneFormat;

        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;


        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(
                    m_bloomExtractTexture
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create bloom extraction target"
        );


        core::ThrowIfFailed(
            m_bloomExtractTexture->SetName(
                L"Gravity Forge Bloom Extract"
            ),
            "Failed to name bloom extraction target"
        );

        D3D12_RENDER_TARGET_VIEW_DESC
            rtvDescription{};

        rtvDescription.Format =
            HdrSceneFormat;

        rtvDescription.ViewDimension =
            D3D12_RTV_DIMENSION_TEXTURE2D;

        rtvDescription.Texture2D.MipSlice = 0;

        rtvDescription.Texture2D.PlaneSlice = 0;


        m_device->CreateRenderTargetView(
            m_bloomExtractTexture.Get(),
            &rtvDescription,
            m_bloomExtractRtvHandle
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC
            srvDescription{};

        srvDescription.Format =
            HdrSceneFormat;

        srvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_TEXTURE2D;

        srvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDescription.Texture2D.MostDetailedMip =
            0;

        srvDescription.Texture2D.MipLevels =
            1;

        srvDescription.Texture2D.PlaneSlice =
            0;

        srvDescription.Texture2D.ResourceMinLODClamp =
            0.0f;


        m_device->CreateShaderResourceView(
            m_bloomExtractTexture.Get(),
            &srvDescription,
            m_bloomExtractSrv.cpu
        );


        core::LogInfo(
            "Half-resolution bloom extraction "
            "target, RTV, and SRV created"
        );
    }

    void D3D12Context::CreateBloomBlurTempTarget()
    {
        if (
            m_width == 0 ||
            m_height == 0)
        {
            throw std::runtime_error(
                "Cannot create bloom blur target "
                "with zero dimensions"
            );
        }


        const UINT bloomWidth =
            std::max(
                1u,
                m_width / 2u
            );

        const UINT bloomHeight =
            std::max(
                1u,
                m_height / 2u
            );


        D3D12_HEAP_PROPERTIES
            heapProperties{};

        heapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;


        D3D12_RESOURCE_DESC
            description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        description.Alignment = 0;

        description.Width =
            bloomWidth;

        description.Height =
            bloomHeight;

        description.DepthOrArraySize = 1;
        description.MipLevels = 1;

        description.Format =
            HdrSceneFormat;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_UNKNOWN;

        description.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;


        D3D12_CLEAR_VALUE
            clearValue{};

        clearValue.Format =
            HdrSceneFormat;

        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;


        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(
                    m_bloomBlurTempTexture
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create bloom blur "
            "temporary target"
        );


        core::ThrowIfFailed(
            m_bloomBlurTempTexture->SetName(
                L"Gravity Forge Bloom Blur Temp"
            ),
            "Failed to name bloom blur "
            "temporary target"
        );

        D3D12_RENDER_TARGET_VIEW_DESC
            rtvDescription{};

        rtvDescription.Format =
            HdrSceneFormat;

        rtvDescription.ViewDimension =
            D3D12_RTV_DIMENSION_TEXTURE2D;

        rtvDescription.Texture2D.MipSlice = 0;
        rtvDescription.Texture2D.PlaneSlice = 0;


        m_device->CreateRenderTargetView(
            m_bloomBlurTempTexture.Get(),
            &rtvDescription,
            m_bloomBlurTempRtvHandle
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC
            srvDescription{};

        srvDescription.Format =
            HdrSceneFormat;

        srvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_TEXTURE2D;

        srvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDescription.Texture2D.MostDetailedMip =
            0;

        srvDescription.Texture2D.MipLevels =
            1;

        srvDescription.Texture2D.PlaneSlice =
            0;

        srvDescription.Texture2D.ResourceMinLODClamp =
            0.0f;


        m_device->CreateShaderResourceView(
            m_bloomBlurTempTexture.Get(),
            &srvDescription,
            m_bloomBlurTempSrv.cpu
        );


        core::LogInfo(
            "Half-resolution bloom blur "
            "temporary target created"
        );
    }

    void D3D12Context::CreateParticleSystem()
    {
        auto* commandAllocator =
            m_frameResources[0]
            .CommandAllocator();

        core::ThrowIfFailed(
            commandAllocator->Reset(),
            "Failed to reset allocator "
            "for particle initialization"
        );

        core::ThrowIfFailed(
            m_commandList->Reset(
                commandAllocator,
                nullptr
            ),
            "Failed to reset command list "
            "for particle initialization"
        );

        m_particleSystem =
            std::make_unique<
            gf::simulation::ParticleSystem
            >(
                m_device.Get(),
                m_commandList.Get(),
                m_galaxyParameters
            );

        m_particleDebugDrawCount =
            m_particleSystem->Count();

        m_particleDensityMultiplier = 1.0f;
        m_particleMotionSpeedMultiplier = 1.0f;

        core::ThrowIfFailed(
            m_commandList->Close(),
            "Failed to close particle "
            "initialization command list"
        );

        ID3D12CommandList*
            commandLists[]{
                m_commandList.Get()
        };

        m_commandQueue->ExecuteCommandLists(
            1,
            commandLists
        );

        FlushGpu();

        m_particleSystem
            ->ReleaseUploadResource();

        core::LogInfo(
            "Particle state buffers "
            "initialized on GPU"
        );
    }

    void D3D12Context::CreateParticleBufferViews()
    {
        if (m_particleSystem == nullptr)
        {
            throw std::runtime_error(
                "Cannot create particle views "
                "before ParticleSystem"
            );
        }


        D3D12_SHADER_RESOURCE_VIEW_DESC
            srvDescription{};

        srvDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        srvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_BUFFER;

        srvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDescription.Buffer.FirstElement = 0;

        srvDescription.Buffer.NumElements =
            m_particleSystem->Count();

        srvDescription.Buffer.StructureByteStride =
            sizeof(
                gf::simulation::ParticleState
                );

        srvDescription.Buffer.Flags =
            D3D12_BUFFER_SRV_FLAG_NONE;


        D3D12_UNORDERED_ACCESS_VIEW_DESC
            uavDescription{};

        uavDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        uavDescription.ViewDimension =
            D3D12_UAV_DIMENSION_BUFFER;

        uavDescription.Buffer.FirstElement = 0;

        uavDescription.Buffer.NumElements =
            m_particleSystem->Count();

        uavDescription.Buffer.StructureByteStride =
            sizeof(
                gf::simulation::ParticleState
                );

        uavDescription.Buffer.CounterOffsetInBytes = 0;

        uavDescription.Buffer.Flags =
            D3D12_BUFFER_UAV_FLAG_NONE;


        for (
            std::uint32_t bufferIndex = 0;
            bufferIndex <
            gf::simulation::ParticleSystem::
            StateBufferCount;
            ++bufferIndex)
        {
            ID3D12Resource* particleBuffer =
                m_particleSystem->StateBuffer(
                    bufferIndex
                );

            if (particleBuffer == nullptr)
            {
                throw std::runtime_error(
                    "Particle state buffer is missing"
                );
            }


            ParticleBufferViews& views =
                m_particleBufferViews[
                    bufferIndex
                ];


            views.srv =
                AllocateSrv();

            m_device->CreateShaderResourceView(
                particleBuffer,
                &srvDescription,
                views.srv.cpu
            );


            views.uav =
                AllocateSrv();

            m_device->CreateUnorderedAccessView(
                particleBuffer,
                nullptr,
                &uavDescription,
                views.uav.cpu
            );
        }

        core::LogInfo(
            "Particle state SRV/UAV views created"
        );
    }

    void D3D12Context::CreatePipeline()
    {
        m_basicPipeline = std::make_unique<gf::graphics::pipeline::BasicPipeline>(m_device.Get(), HdrSceneFormat);

        m_bloomExtractPipeline = std::make_unique<gf::graphics::pipeline::BloomExtractPipeline>(m_device.Get(),HdrSceneFormat);

        m_presentationPipeline = std::make_unique<gf::graphics::pipeline::PresentationPipeline>(m_device.Get(),BackBufferFormat);

        m_particleSimulationPipeline = std::make_unique<gf::graphics::pipeline::ParticleSimulationPipeline>(m_device.Get());

        m_particleDebugPipeline = std::make_unique<gf::graphics::pipeline::ParticleDebugPipeline>(m_device.Get(),HdrSceneFormat,DepthFormat);

        m_particleBillboardPipeline = std::make_unique<gf::graphics::pipeline::ParticleBillboardPipeline>(m_device.Get(),HdrSceneFormat,DepthFormat);

        m_debrisCullingPipeline = std::make_unique<gf::graphics::pipeline::DebrisCullingPipeline>(m_device.Get());

        m_bloomBlurPipeline = std::make_unique<gf::graphics::pipeline::BloomBlurPipeline>(m_device.Get(),HdrSceneFormat);
        
        core::LogInfo(
            "Scene, presentation, particle simulation, "
            "particle debug, and particle billboard "
            "pipelines created"
        );
    }

    void D3D12Context::UpdateViewportAndScissor()
    {
        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;

        m_viewport.Width =
            static_cast<float>(m_width);

        m_viewport.Height =
            static_cast<float>(m_height);

        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        m_scissorRect.left = 0;
        m_scissorRect.top = 0;

        m_scissorRect.right =
            static_cast<LONG>(m_width);

        m_scissorRect.bottom =
            static_cast<LONG>(m_height);
    }

    void D3D12Context::CreateCommandObjects()
    {
        m_frameResources.reserve(
            FrameResourceCount
        );

        for (UINT index = 0; index < FrameResourceCount; ++index)
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;

            core::ThrowIfFailed(
                m_device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(
                        allocator.ReleaseAndGetAddressOf()
                    )
                ),
                "ID3D12Device::"
                "CreateCommandAllocator failed"
            );

            const std::wstring allocatorName =
                std::format(
                    L"Gravity Forge Frame {} "
                    L"Command Allocator",
                    index
                );

            core::ThrowIfFailed(
                allocator->SetName(
                    allocatorName.c_str()
                ),
                "Failed to name frame "
                "command allocator"
            );

            m_frameResources.emplace_back(
                m_device.Get(),
                std::move(allocator),
                index
            );
        }

        core::ThrowIfFailed(
            m_device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                m_frameResources[0].CommandAllocator(),
                nullptr,
                IID_PPV_ARGS(
                    m_commandList.ReleaseAndGetAddressOf()
                )
            ),
            "ID3D12Device::CreateCommandList failed"
        );

        core::ThrowIfFailed(
            m_commandList->SetName(
                L"Gravity Forge Main Graphics Command List"
            ),
            "Failed to name graphics command list"
        );

        core::ThrowIfFailed(
            m_commandList->Close(),
            "Initial command-list Close failed"
        );

        core::LogInfo(
            "Command allocators and command list created"
        );
    }

    void D3D12Context::CreateSynchronizationObjects()
    {
        core::ThrowIfFailed(
            m_device->CreateFence(
                0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(
                    m_fence.ReleaseAndGetAddressOf()
                )
            ),
            "ID3D12Device::CreateFence failed"
        );

        core::ThrowIfFailed(
            m_fence->SetName(
                L"Gravity Forge Graphics Fence"
            ),
            "Failed to name graphics fence"
        );

        m_fenceEvent = CreateEventW(
            nullptr,
            FALSE,
            FALSE,
            nullptr
        );

        if (m_fenceEvent == nullptr)
        {
            core::ThrowLastWin32Error(
                "CreateEventW for graphics fence failed"
            );
        }

        core::LogInfo(
            "Graphics fence and event created"
        );
    }

    void D3D12Context::CreateReactorMeshes()
    {
        auto* commandAllocator =
            m_frameResources[0].CommandAllocator();

        core::ThrowIfFailed(
            commandAllocator->Reset(),
            "Failed to reset allocator "
            "for reactor mesh upload"
        );

        core::ThrowIfFailed(
            m_commandList->Reset(
                commandAllocator,
                nullptr
            ),
            "Failed to reset command list "
            "for reactor mesh upload"
        );

        const gf::graphics::mesh::MeshData
            ringData =
            gf::graphics::mesh::CreateTorusMesh(
                1.0f,
                0.12f,
                48,
                16
            );

        const gf::graphics::mesh::MeshData
            coreData =
            gf::graphics::mesh::CreateUvSphereMesh(
                0.42f,
                24,
                32
            );


        m_ringMesh =
            std::make_unique<
            gf::graphics::mesh::Mesh
            >(
                m_device.Get(),
                m_commandList.Get(),
                std::span<const gf::graphics::mesh::Vertex>{
            ringData.vertices.data(),
                ringData.vertices.size()
        },
                std::span<const std::uint16_t>{
            ringData.indices.data(),
                ringData.indices.size()
        },
                L"Gravity Forge Reactor Ring"
            );

        m_coreMesh =
            std::make_unique<
            gf::graphics::mesh::Mesh
            >(
                m_device.Get(),
                m_commandList.Get(),
                std::span<const gf::graphics::mesh::Vertex>{
            coreData.vertices.data(),
                coreData.vertices.size()
        },
                std::span<const std::uint16_t>{
            coreData.indices.data(),
                coreData.indices.size()
        },
                L"Gravity Forge Reactor Core"
            );

        core::ThrowIfFailed(
            m_commandList->Close(),
            "Failed to close reactor mesh "
            "upload command list"
        );

        ID3D12CommandList* commandLists[]
        {
            m_commandList.Get()
        };

        m_commandQueue->ExecuteCommandLists(
            1,
            commandLists
        );

        FlushGpu();

        m_ringMesh->ReleaseUploadResources();
        m_coreMesh->ReleaseUploadResources();

        core::LogInfo(
            "Reactor meshes uploaded"
        );
    }

    void D3D12Context::CreateDebrisField()
    {
        auto* commandAllocator =
            m_frameResources[0].CommandAllocator();

        core::ThrowIfFailed(
            commandAllocator->Reset(),
            "Failed to reset allocator for debris upload"
        );

        core::ThrowIfFailed(
            m_commandList->Reset(
                commandAllocator,
                nullptr
            ),
            "Failed to reset command list for debris upload"
        );

        m_debrisField =
            std::make_unique<
            gf::scene::DebrisField
            >(
                m_device.Get(),
                m_commandList.Get()
            );

        m_debrisDrawCount =
            m_debrisField->Count();

        core::ThrowIfFailed(
            m_commandList->Close(),
            "Failed to close debris upload command list"
        );

        ID3D12CommandList* commandLists[]{
            m_commandList.Get()
        };

        m_commandQueue->ExecuteCommandLists(
            1,
            commandLists
        );

        FlushGpu();

        m_debrisField->ReleaseUploadResource();

        core::LogInfo(
            "Debris instance buffer uploaded"
        );
    }

    void D3D12Context::CreateDebrisInstanceView()
    {
        m_debrisInstancesSrv =
            AllocateSrv();

        D3D12_SHADER_RESOURCE_VIEW_DESC
            description{};

        description.Format =
            DXGI_FORMAT_UNKNOWN;

        description.ViewDimension =
            D3D12_SRV_DIMENSION_BUFFER;

        description.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        description.Buffer.FirstElement = 0;

        description.Buffer.NumElements =
            m_debrisField->Count();

        description.Buffer.StructureByteStride =
            sizeof(gf::scene::DebrisInstance);

        description.Buffer.Flags =
            D3D12_BUFFER_SRV_FLAG_NONE;

        m_device->CreateShaderResourceView(
            m_debrisField->InstanceBuffer(),
            &description,
            m_debrisInstancesSrv.cpu
        );
    }

    void D3D12Context::
        CreateDebrisCullingResources()
    {
        if (m_debrisField == nullptr)
        {
            throw std::runtime_error(
                "Cannot create debris culling "
                "resources before DebrisField"
            );
        }

        const UINT64 bufferSize =
            static_cast<UINT64>(
                m_debrisField->Count()
                ) *
            sizeof(std::uint32_t);

        D3D12_HEAP_PROPERTIES
            heapProperties{};

        heapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        heapProperties.CPUPageProperty =
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

        heapProperties.MemoryPoolPreference =
            D3D12_MEMORY_POOL_UNKNOWN;

        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;


        D3D12_RESOURCE_DESC description{};

        description.Dimension =
            D3D12_RESOURCE_DIMENSION_BUFFER;

        description.Alignment = 0;
        description.Width = bufferSize;
        description.Height = 1;
        description.DepthOrArraySize = 1;
        description.MipLevels = 1;

        description.Format =
            DXGI_FORMAT_UNKNOWN;

        description.SampleDesc.Count = 1;
        description.SampleDesc.Quality = 0;

        description.Layout =
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        description.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_PPV_ARGS(
                    m_debrisVisibilityFlags
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris "
            "visibility-flags buffer"
        );

        core::ThrowIfFailed(
            m_debrisVisibilityFlags->SetName(
                L"Gravity Forge Debris "
                L"Visibility Flags"
            ),
            "Failed to name debris "
            "visibility-flags buffer"
        );

        description.Width = bufferSize;

        description.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_PPV_ARGS(
                    m_debrisVisibleInstanceIndices
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris visible-instance "
            "indices buffer"
        );

        core::ThrowIfFailed(
            m_debrisVisibleInstanceIndices->SetName(
                L"Gravity Forge Debris Visible Instance Indices"
            ),
            "Failed to name debris "
            "Visible Instance Indices buffer"
        );

        m_debrisVisibleInstanceIndicesUav =
            AllocateSrv();

        D3D12_UNORDERED_ACCESS_VIEW_DESC
            visibleIndicesUavDescription{};

        visibleIndicesUavDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        visibleIndicesUavDescription.ViewDimension =
            D3D12_UAV_DIMENSION_BUFFER;

        visibleIndicesUavDescription.Buffer.FirstElement = 0;

        visibleIndicesUavDescription.Buffer.NumElements =
            m_debrisField->Count();

        visibleIndicesUavDescription
            .Buffer
            .StructureByteStride =
            sizeof(std::uint32_t);

        visibleIndicesUavDescription.Buffer.Flags =
            D3D12_BUFFER_UAV_FLAG_NONE;

        m_device->CreateUnorderedAccessView(
            m_debrisVisibleInstanceIndices.Get(),
            nullptr,
            &visibleIndicesUavDescription,
            m_debrisVisibleInstanceIndicesUav.cpu
        );

        m_debrisVisibleInstanceIndicesSrv =
            AllocateSrv();

        D3D12_SHADER_RESOURCE_VIEW_DESC
            visibleIndicesSrvDescription{};

        visibleIndicesSrvDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        visibleIndicesSrvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_BUFFER;

        visibleIndicesSrvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        visibleIndicesSrvDescription.Buffer.FirstElement = 0;

        visibleIndicesSrvDescription.Buffer.NumElements =
            m_debrisField->Count();

        visibleIndicesSrvDescription
            .Buffer
            .StructureByteStride =
            sizeof(std::uint32_t);

        visibleIndicesSrvDescription.Buffer.Flags =
            D3D12_BUFFER_SRV_FLAG_NONE;

        m_device->CreateShaderResourceView(
            m_debrisVisibleInstanceIndices.Get(),
            &visibleIndicesSrvDescription,
            m_debrisVisibleInstanceIndicesSrv.cpu
        );

        description.Width =
            sizeof(std::uint32_t);

        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &description,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_PPV_ARGS(
                    m_debrisVisibleCount
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris visible-count buffer"
        );

        core::ThrowIfFailed(
            m_debrisVisibleCount->SetName(
                L"Gravity Forge Debris Visible Count"
            ),
            "Failed to name debris "
            "Debris Visible Count buffer"
        );

        m_debrisVisibleCountUav =
            AllocateSrv();

        D3D12_UNORDERED_ACCESS_VIEW_DESC
            visibleCountUavDescription{};

        visibleCountUavDescription.Format =
            DXGI_FORMAT_R32_UINT;

        visibleCountUavDescription.ViewDimension =
            D3D12_UAV_DIMENSION_BUFFER;

        visibleCountUavDescription.Buffer.FirstElement = 0;
        visibleCountUavDescription.Buffer.NumElements = 1;
        visibleCountUavDescription.Buffer.StructureByteStride = 0;
        visibleCountUavDescription.Buffer.Flags =
            D3D12_BUFFER_UAV_FLAG_NONE;

        m_device->CreateUnorderedAccessView(
            m_debrisVisibleCount.Get(),
            nullptr,
            &visibleCountUavDescription,
            m_debrisVisibleCountUav.cpu
        );


        m_debrisVisibleCountSrv =
            AllocateSrv();

        D3D12_SHADER_RESOURCE_VIEW_DESC
            visibleCountSrvDescription{};

        visibleCountSrvDescription.Format =
            DXGI_FORMAT_R32_UINT;

        visibleCountSrvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_BUFFER;

        visibleCountSrvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        visibleCountSrvDescription.Buffer.FirstElement = 0;
        visibleCountSrvDescription.Buffer.NumElements = 1;
        visibleCountSrvDescription.Buffer.StructureByteStride = 0;
        visibleCountSrvDescription.Buffer.Flags =
            D3D12_BUFFER_SRV_FLAG_NONE;

        m_device->CreateShaderResourceView(
            m_debrisVisibleCount.Get(),
            &visibleCountSrvDescription,
            m_debrisVisibleCountSrv.cpu
        );

        D3D12_DESCRIPTOR_HEAP_DESC
            clearHeapDescription{};

        clearHeapDescription.Type =
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        clearHeapDescription.NumDescriptors = 1;

        clearHeapDescription.Flags =
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        core::ThrowIfFailed(
            m_device->CreateDescriptorHeap(
                &clearHeapDescription,
                IID_PPV_ARGS(
                    m_debrisVisibleCountClearHeap
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris visible-count "
            "clear descriptor heap"
        );

        m_device->CreateUnorderedAccessView(
            m_debrisVisibleCount.Get(),
            nullptr,
            &visibleCountUavDescription,
            m_debrisVisibleCountClearHeap
            ->GetCPUDescriptorHandleForHeapStart()
        );

        m_debrisVisibilityFlagsUav =
            AllocateSrv();

        D3D12_UNORDERED_ACCESS_VIEW_DESC
            uavDescription{};

        uavDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        uavDescription.ViewDimension =
            D3D12_UAV_DIMENSION_BUFFER;

        uavDescription.Buffer.FirstElement = 0;

        uavDescription.Buffer.NumElements =
            m_debrisField->Count();

        uavDescription
            .Buffer
            .StructureByteStride =
            sizeof(std::uint32_t);

        uavDescription
            .Buffer
            .CounterOffsetInBytes = 0;

        uavDescription.Buffer.Flags =
            D3D12_BUFFER_UAV_FLAG_NONE;

        m_device->CreateUnorderedAccessView(
            m_debrisVisibilityFlags.Get(),
            nullptr,
            &uavDescription,
            m_debrisVisibilityFlagsUav.cpu
        );

        m_debrisVisibilityFlagsSrv =
            AllocateSrv();

        D3D12_SHADER_RESOURCE_VIEW_DESC
            flagsSrvDescription{};

        flagsSrvDescription.Format =
            DXGI_FORMAT_UNKNOWN;

        flagsSrvDescription.ViewDimension =
            D3D12_SRV_DIMENSION_BUFFER;

        flagsSrvDescription.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        flagsSrvDescription.Buffer.FirstElement = 0;

        flagsSrvDescription.Buffer.NumElements =
            m_debrisField->Count();

        flagsSrvDescription
            .Buffer
            .StructureByteStride =
            sizeof(std::uint32_t);

        flagsSrvDescription.Buffer.Flags =
            D3D12_BUFFER_SRV_FLAG_NONE;

        m_device->CreateShaderResourceView(
            m_debrisVisibilityFlags.Get(),
            &flagsSrvDescription,
            m_debrisVisibilityFlagsSrv.cpu
        );

        core::LogInfo(
            "Debris visibility-flags buffer created"
        );
    }

    void D3D12Context::CreateDebrisIndirectResources()
    {
        if (m_braceMesh == nullptr)
        {
            throw std::runtime_error(
                "Cannot create debris indirect resources "
                "before debris mesh"
            );
        }

        D3D12_INDIRECT_ARGUMENT_DESC
            indirectArgument{};

        indirectArgument.Type =
            D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC
            commandSignatureDescription{};

        commandSignatureDescription.ByteStride =
            sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

        commandSignatureDescription.NumArgumentDescs = 1;

        commandSignatureDescription.pArgumentDescs =
            &indirectArgument;

        core::ThrowIfFailed(
            m_device->CreateCommandSignature(
                &commandSignatureDescription,
                nullptr,
                IID_PPV_ARGS(
                    m_debrisDrawCommandSignature
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris indirect "
            "draw command signature"
        );

        core::ThrowIfFailed(
            m_debrisDrawCommandSignature->SetName(
                L"Gravity Forge Debris Draw "
                L"Command Signature"
            ),
            "Failed to name debris indirect "
            "command signature"
        );

        D3D12_DRAW_INDEXED_ARGUMENTS
            initialArguments{};

        initialArguments.IndexCountPerInstance =
            m_braceMesh->IndexCount();

        initialArguments.InstanceCount = 0;

        initialArguments.StartIndexLocation = 0;
        initialArguments.BaseVertexLocation = 0;
        initialArguments.StartInstanceLocation = 0;

        D3D12_HEAP_PROPERTIES
            defaultHeapProperties{};

        defaultHeapProperties.Type =
            D3D12_HEAP_TYPE_DEFAULT;

        defaultHeapProperties.CreationNodeMask = 1;
        defaultHeapProperties.VisibleNodeMask = 1;


        D3D12_RESOURCE_DESC
            bufferDescription{};

        bufferDescription.Dimension =
            D3D12_RESOURCE_DIMENSION_BUFFER;

        bufferDescription.Width =
            sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

        bufferDescription.Height = 1;
        bufferDescription.DepthOrArraySize = 1;
        bufferDescription.MipLevels = 1;

        bufferDescription.SampleDesc.Count = 1;

        bufferDescription.Layout =
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDescription,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(
                    m_debrisIndirectArguments
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris indirect "
            "argument buffer"
        );

        core::ThrowIfFailed(
            m_debrisIndirectArguments->SetName(
                L"Gravity Forge Debris "
                L"Indirect Arguments"
            ),
            "Failed to name debris indirect "
            "argument buffer"
        );

        D3D12_HEAP_PROPERTIES
            uploadHeapProperties{};

        uploadHeapProperties.Type =
            D3D12_HEAP_TYPE_UPLOAD;

        uploadHeapProperties.CreationNodeMask = 1;
        uploadHeapProperties.VisibleNodeMask = 1;

        Microsoft::WRL::ComPtr<ID3D12Resource>
            uploadBuffer;

        core::ThrowIfFailed(
            m_device->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(
                    uploadBuffer
                    .ReleaseAndGetAddressOf()
                )
            ),
            "Failed to create debris indirect "
            "argument upload buffer"
        );

        void* mappedMemory = nullptr;

        const D3D12_RANGE cpuReadRange{
            0,
            0
        };

        core::ThrowIfFailed(
            uploadBuffer->Map(
                0,
                &cpuReadRange,
                &mappedMemory
            ),
            "Failed to map debris indirect "
            "argument upload buffer"
        );

        std::memcpy(
            mappedMemory,
            &initialArguments,
            sizeof(initialArguments)
        );

        uploadBuffer->Unmap(
            0,
            nullptr
        );

        auto* commandAllocator =
            m_frameResources[0].CommandAllocator();

        core::ThrowIfFailed(
            commandAllocator->Reset(),
            "Failed to reset allocator for "
            "debris indirect argument upload"
        );

        core::ThrowIfFailed(
            m_commandList->Reset(
                commandAllocator,
                nullptr
            ),
            "Failed to reset command list for "
            "debris indirect argument upload"
        );

        m_commandList->CopyBufferRegion(
            m_debrisIndirectArguments.Get(),
            0,
            uploadBuffer.Get(),
            0,
            sizeof(initialArguments)
        );

        D3D12_RESOURCE_BARRIER
            toIndirectArgument{};

        toIndirectArgument.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        toIndirectArgument.Transition.pResource =
            m_debrisIndirectArguments.Get();

        toIndirectArgument.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        toIndirectArgument.Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;

        toIndirectArgument.Transition.StateAfter =
            D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

        m_commandList->ResourceBarrier(
            1,
            &toIndirectArgument
        );

        core::ThrowIfFailed(
            m_commandList->Close(),
            "Failed to close debris indirect "
            "argument upload command list"
        );

        ID3D12CommandList* commandLists[]
        {
            m_commandList.Get()
        };

        m_commandQueue->ExecuteCommandLists(
            1,
            commandLists
        );

        FlushGpu();

        core::LogInfo(
            "Debris indirect draw resources created"
        );
    }

    const gf::graphics::mesh::Mesh&
        D3D12Context::MeshFor(
            gf::scene::ReactorMeshKind kind
        ) const
    {
        switch (kind)
        {
        case gf::scene::ReactorMeshKind::Ring:
            return *m_ringMesh;

        case gf::scene::ReactorMeshKind::Core:
            return *m_coreMesh;
        }

        throw std::logic_error(
            "Unknown reactor mesh kind"
        );
    }

    const gf::graphics::material::Material&
        D3D12Context::MaterialFor(
            gf::scene::ReactorMaterialKind kind
        ) const
    {
        switch (kind)
        {
        case gf::scene::ReactorMaterialKind::Metal:
            return m_loadedMaterial;

        case gf::scene::ReactorMaterialKind::Core:
            return m_coreMaterial;
        }

        throw std::logic_error(
            "Unknown reactor material kind"
        );
    }

    void D3D12Context::CreateTestTexture()
    {
        auto* commandAllocator =
            m_frameResources[0].CommandAllocator();

        core::ThrowIfFailed(
            commandAllocator->Reset(),
            "Failed to reset allocator "
            "for texture uploads"
        );

        core::ThrowIfFailed(
            m_commandList->Reset(
                commandAllocator,
                nullptr
            ),
            "Failed to reset command list "
            "for texture uploads"
        );

        const gf::graphics::texture::Rgba8Image
            uvTestImage =
            gf::graphics::texture::CreateUvTestImage();

        const std::filesystem::path materialPath =
            std::filesystem::path{
                GF_ASSET_ROOT
        }
            / "textures"
            / "material.png";

        const gf::graphics::texture::Rgba8Image
            decodedMaterialImage =
            gf::graphics::texture::LoadRgba8Image(
                materialPath
            );

        m_testTexture =
            std::make_unique<
            gf::graphics::texture::Texture2D
            >(
                m_device.Get(),
                m_commandList.Get(),
                uvTestImage,
                L"Gravity Forge UV Test Texture"
            );

        m_materialTexture =
            std::make_unique<
            gf::graphics::texture::Texture2D
            >(
                m_device.Get(),
                m_commandList.Get(),
                decodedMaterialImage,
                L"Gravity Forge Decoded Material Texture"
            );

        core::ThrowIfFailed(
            m_commandList->Close(),
            "Failed to close texture upload "
            "command list"
        );

        ID3D12CommandList* commandLists[]
        {
            m_commandList.Get()
        };

        m_commandQueue->ExecuteCommandLists(
            1,
            commandLists
        );

        FlushGpu();

        m_testTexture->ReleaseUploadResource();
        m_materialTexture->ReleaseUploadResource();

        core::LogInfo(
            "UV and decoded material textures uploaded"
        );
    }

    D3D12_GPU_DESCRIPTOR_HANDLE
        D3D12Context::CreateTextureSrv(
            const gf::graphics::texture::Texture2D&
            texture)
    {
        const SrvAllocation allocation =
            AllocateSrv();

        D3D12_SHADER_RESOURCE_VIEW_DESC
            description{};

        description.Format =
            texture.Format();

        description.ViewDimension =
            D3D12_SRV_DIMENSION_TEXTURE2D;

        description.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        description.Texture2D.MostDetailedMip = 0;
        description.Texture2D.MipLevels = 1;
        description.Texture2D.PlaneSlice = 0;
        description.Texture2D.ResourceMinLODClamp = 0.0f;

        m_device->CreateShaderResourceView(
            texture.Resource(),
            &description,
            allocation.cpu
        );

        return allocation.gpu;
    }

    void D3D12Context::CreateTestTextureSrvs()
    {
        if (m_testTexture == nullptr ||
            m_materialTexture == nullptr)
        {
            throw std::runtime_error(
                "Cannot create SRVs before textures"
            );
        }

        m_coreMaterial.baseColorSrv =
            CreateTextureSrv(
                *m_testTexture
            );

        m_coreMaterial.colorTint =
            DirectX::XMFLOAT4{
                0.10f,
                0.65f,
                1.0f,
                1.0f
        };

        m_coreMaterial.metallic = 0.0f;
        m_coreMaterial.roughness = 0.20f;

        m_coreMaterial.emissiveIntensity = 3.5f;
        m_coreMaterial.pulseSpeed = 0.75f;

        m_loadedMaterial.baseColorSrv =
            CreateTextureSrv(
                *m_materialTexture
            );

        m_loadedMaterial.colorTint =
            DirectX::XMFLOAT4{
                0.75f,
                0.88f,
                1.0f,
                1.0f
        };

        m_loadedMaterial.metallic = 0.80f;
        m_loadedMaterial.roughness = 0.18f;

        m_loadedMaterial.emissiveIntensity = 0.0f;
        m_loadedMaterial.pulseSpeed = 1.0f;

        core::LogInfo(
            "Texture SRVs and materials created"
        );
    }

    void D3D12Context::WaitForFrame(
        const gf::graphics::frame::FrameResource&
        frameResource
    )
    {
        const UINT64 requiredFenceValue = frameResource.FenceValue();

        if (requiredFenceValue == 0)
        {
            return;
        }

        if (m_fence->GetCompletedValue() >= requiredFenceValue)
        {
            return;
        }

        core::ThrowIfFailed(
            m_fence->SetEventOnCompletion(
                requiredFenceValue,
                m_fenceEvent
            ),
            "ID3D12Fence::SetEventOnCompletion failed"
        );

        const DWORD waitResult =
            WaitForSingleObject(
                m_fenceEvent,
                INFINITE
            );

        if (waitResult == WAIT_FAILED)
        {
            core::ThrowLastWin32Error(
                "Waiting for frame fence failed"
            );
        }

        if (waitResult != WAIT_OBJECT_0)
        {
            throw std::runtime_error(
                "Unexpected result while waiting for frame fence"
            );
        }
    }

    void D3D12Context::DrawMesh(
        const gf::graphics::mesh::Mesh& mesh,
        gf::graphics::frame::FrameResource&
        frameResource,
        const gf::scene::Camera& camera,
        const DirectX::XMMATRIX& model,
        const gf::graphics::material::Material&
        material,
        float elapsedSeconds,
        std::uint32_t objectIndex
    )
    {
        const D3D12_VERTEX_BUFFER_VIEW&
            vertexBufferView =
            mesh.VertexBufferView();

        const D3D12_INDEX_BUFFER_VIEW&
            indexBufferView =
            mesh.IndexBufferView();

        m_commandList->IASetVertexBuffers(
            0,
            1,
            &vertexBufferView
        );

        m_commandList->IASetIndexBuffer(
            &indexBufferView
        );

        const DirectX::XMMATRIX
            modelViewProjection =
            model *
            camera.ViewMatrix() *
            camera.ProjectionMatrix();

        gf::graphics::frame::SceneConstants
            sceneConstants{};

        DirectX::XMStoreFloat4x4(
            &sceneConstants.modelViewProjection,
            modelViewProjection
        );

        sceneConstants.colorTint =
            material.colorTint;

        sceneConstants.materialParameters =
            DirectX::XMFLOAT2{
                material.metallic,
                material.roughness
        };

        sceneConstants.animationParameters =
            DirectX::XMFLOAT4{
                elapsedSeconds,
                material.emissiveIntensity,
                material.pulseSpeed,
                0.0f
        };

        frameResource.WriteSceneConstants(
            objectIndex,
            sceneConstants
        );

        m_commandList->SetGraphicsRootDescriptorTable(
            1,
            material.baseColorSrv
        );

        m_commandList
            ->SetGraphicsRootConstantBufferView(
                0,
                frameResource
                .SceneConstantBufferAddress(
                    objectIndex
                )
            );

        m_commandList->DrawIndexedInstanced(
            mesh.IndexCount(),
            1,
            0,
            0,
            0
        );
    }

    void D3D12Context::DrawDebris(
        gf::graphics::frame::FrameResource&
        frameResource,
        const gf::scene::Camera& camera,
        float animationSeconds,
        std::uint32_t constantSlot)
    {
        if (m_debrisDrawCount == 0)
        {
            return;
        }

        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(3),
            m_debrisCullingDebugEnabled
            ? L"Debris Draw [Culling Debug]"
            : (
                m_debrisCullingEnabled
                ? L"Debris Draw [GPU Culled]"
                : L"Debris Draw [All Instances]"
            )
        );

        // Legacy debris draw path uses a legacy mesh.
        const D3D12_VERTEX_BUFFER_VIEW&
            vertexBufferView =
            m_braceMesh->VertexBufferView();

        const D3D12_INDEX_BUFFER_VIEW&
            indexBufferView =
            m_braceMesh->IndexBufferView();

        m_commandList->IASetVertexBuffers(
            0,
            1,
            &vertexBufferView
        );

        m_commandList->IASetIndexBuffer(
            &indexBufferView
        );

        gf::graphics::frame::SceneConstants
            constants{};

        const DirectX::XMMATRIX viewProjection =
            camera.ViewMatrix() *
            camera.ProjectionMatrix();

        DirectX::XMStoreFloat4x4(
            &constants.modelViewProjection,
            viewProjection
        );

        constants.colorTint =
            m_loadedMaterial.colorTint;

        constants.materialParameters = {
            m_loadedMaterial.metallic,
            m_loadedMaterial.roughness
        };

        const float debrisRenderMode =
            m_debrisCullingDebugEnabled
            ? 2.0f
            : (
                m_debrisCullingEnabled
                ? 1.0f
                : 0.0f
            );

        constants.animationParameters = {
            animationSeconds,
            debrisRenderMode,
            0.0f,
            0.0f
        };

        frameResource.WriteSceneConstants(
            constantSlot,
            constants
        );

        m_commandList
            ->SetGraphicsRootConstantBufferView(
                0,
                frameResource
                .SceneConstantBufferAddress(
                    constantSlot
                )
            );

        m_commandList->SetGraphicsRootDescriptorTable(
            1,
            m_loadedMaterial.baseColorSrv
        );

        m_commandList->SetGraphicsRootDescriptorTable(
            2,
            m_debrisInstancesSrv.gpu
        );

        m_commandList->SetGraphicsRootDescriptorTable(
            3,
            m_debrisVisibleInstanceIndicesSrv.gpu
        );

        m_commandList->SetGraphicsRootDescriptorTable(
            4,
            m_debrisVisibleCountSrv.gpu
        );

        m_commandList->SetGraphicsRootDescriptorTable(
            5,
            m_debrisVisibilityFlagsSrv.gpu
        );

        const bool useIndirect =
            m_debrisCullingEnabled &&
            !m_debrisCullingDebugEnabled &&
            m_debrisIndirectEnabled;

        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(3),

            m_debrisCullingDebugEnabled
            ? L"Debris Draw [Culling Debug]"
            : (
                useIndirect
                ? L"Debris Draw [GPU Culled + Indirect]"
                : (
                    m_debrisCullingEnabled
                    ? L"Debris Draw [GPU Culled + Direct]"
                    : L"Debris Draw [All Instances]"
                    )
                )
        );

        if (useIndirect)
        {
            m_commandList->ExecuteIndirect(
                m_debrisDrawCommandSignature.Get(),

                1,

                m_debrisIndirectArguments.Get(),
                0,

                nullptr,
                0
            );
        }
        else
        {
            m_commandList->DrawIndexedInstanced(
                m_braceMesh->IndexCount(),
                m_debrisDrawCount,
                0,
                0,
                0
            );
        }
    }

    gf::graphics::frame::FrameResource& D3D12Context::BeginFrame()
    {
        gf::graphics::frame::FrameResource&
            frameResource =
            m_frameResources[
                m_frameResourceIndex
            ];

        WaitForFrame(frameResource);

        core::ThrowIfFailed(
            frameResource
            .CommandAllocator()
            ->Reset(),
            "Frame-resource command "
            "allocator Reset failed"
        );

        core::ThrowIfFailed(
            m_commandList->Reset(
                frameResource.CommandAllocator(),
                m_basicPipeline->PipelineState(
                    m_depthTestingEnabled,
                    gf::graphics::pipeline::PipelineKind::Surface
                )
            ),
            "ID3D12GraphicsCommandList::"
            "Reset failed"
        );

        return frameResource;
    }

    void D3D12Context::RecordParticleSimulation(
        float deltaSeconds)
    {
        if (m_particleSystem == nullptr)
        {
            throw std::runtime_error(
                "Particle simulation requires "
                "a ParticleSystem"
            );
        }

        if (m_particleSimulationPipeline == nullptr)
        {
            throw std::runtime_error(
                "Particle simulation requires "
                "a compute pipeline"
            );
        }

        if (!m_particleSimulationEnabled)
        {
            return;
        }

        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(1),
            L"Particle Simulation"
        );

        const std::uint32_t readBufferIndex =
            m_particleSystem
            ->ReadBufferIndex();

        const std::uint32_t writeBufferIndex =
            m_particleSystem
            ->WriteBufferIndex();


        ID3D12Resource* readBuffer =
            m_particleSystem->ReadBuffer();

        ID3D12Resource* writeBuffer =
            m_particleSystem->WriteBuffer();

        if (
            readBuffer == nullptr ||
            writeBuffer == nullptr)
        {
            throw std::runtime_error(
                "Particle simulation buffer "
                "is missing"
            );
        }


        //
        // At the START of every simulation step,
        // our invariant is:
        //
        // readBuffer  = NON_PIXEL_SHADER_RESOURCE
        // writeBuffer = UNORDERED_ACCESS
        //
        // Therefore no pre-dispatch transitions
        // are necessary.
        //

        m_commandList->SetPipelineState(
            m_particleSimulationPipeline
            ->PipelineState()
        );

        m_commandList->SetComputeRootSignature(
            m_particleSimulationPipeline
            ->RootSignature()
        );


        m_commandList
            ->SetComputeRootDescriptorTable(
                0,
                m_particleBufferViews[
                    readBufferIndex
                ].srv.gpu
            );

        m_commandList
            ->SetComputeRootDescriptorTable(
                1,
                m_particleBufferViews[
                    writeBufferIndex
                ].uav.gpu
            );


        const float safeDeltaTime =
            std::clamp(
                deltaSeconds,
                0.0f,
                MaximumParticleDeltaTime
            ) *
            m_particleMotionSpeedMultiplier;

        const float clampedSafeDeltaTime =
            std::clamp(
                safeDeltaTime,
                0.0f,
                MaximumParticleDeltaTime
            );

        const ParticleSimulationMode
            simulationModeForDispatch =
            m_particleSimulationMode;

        ParticleSimulationRootConstants
            simulationConstants{};

        simulationConstants.deltaTime =
            clampedSafeDeltaTime;

        simulationConstants.simulationPadding0 =
            0.0f;

        simulationConstants.softeningSquared =
            ParticleSofteningRadius *
            ParticleSofteningRadius;

        simulationConstants.particleCount =
            m_particleSystem->Count();

        simulationConstants.simulationMode =
            static_cast<std::uint32_t>(
                simulationModeForDispatch
                );

        for (
            std::uint32_t sourceIndex = 0;
            sourceIndex <
            MaxInteractiveGravitySources;
            ++sourceIndex)
        {
            const auto& source =
                m_interactiveGravitySources[
                    sourceIndex
                ];

            auto& gpuSource =
                simulationConstants
                .interactiveGravitySources[
                    sourceIndex
                ];

            gpuSource.positionX =
                source.position.x;

            gpuSource.positionY =
                source.position.y;

            gpuSource.positionZ =
                source.position.z;

            gpuSource.strength =
                source.strength;
        }

        simulationConstants
            .interactiveGravitySourceCount =
            m_interactiveGravityEnabled
            ? m_interactiveGravitySourceCount
            : 0u;

        simulationConstants.simulationMode =
            static_cast<std::uint32_t>(
                m_particleSimulationMode
                );

        simulationConstants.explosionStrength =
            m_particleExplosionStrength;

        simulationConstants
            .explosionTangentialStrength =
            m_particleExplosionTangentialStrength;

        simulationConstants
            .reformPositionStrength =
            m_particleReformPositionStrength;

        simulationConstants
            .reformVelocityStrength =
            m_particleReformVelocityStrength;

        simulationConstants
            .reformMaxAcceleration =
            m_particleReformMaxAcceleration;

        simulationConstants.reformPadding =
            0.0f;

        simulationConstants.galaxyArmCount =
            m_galaxyParameters.armCount;

        simulationConstants.galaxyArmPitch =
            m_galaxyParameters.armPitch;

        simulationConstants.galaxyArmConcentration =
            m_galaxyParameters.armConcentration;

        simulationConstants.galaxyInterArmFraction =
            m_galaxyParameters.interArmFraction;

        simulationConstants
            .galaxyRegularArmHalfWidthInnerDegrees =
            m_galaxyParameters
            .regularArmHalfWidthInnerDegrees;

        simulationConstants
            .galaxyRegularArmHalfWidthOuterDegrees =
            m_galaxyParameters
            .regularArmHalfWidthOuterDegrees;

        simulationConstants
            .galaxyRegularArmHalfWidthRadiusPower =
            m_galaxyParameters
            .regularArmHalfWidthRadiusPower;

        simulationConstants
            .galaxyRegularArmHalfWidthOuterBias =
            m_galaxyParameters
            .regularArmHalfWidthOuterBias;

        simulationConstants
            .galaxyInterArmFractionInner =
            m_galaxyParameters
            .interArmFractionInner;

        simulationConstants
            .galaxyInterArmFractionOuter =
            m_galaxyParameters
            .interArmFractionOuter;

        simulationConstants
            .galaxyInterArmFractionRadiusPower =
            m_galaxyParameters
            .interArmFractionRadiusPower;

        simulationConstants
            .galaxyInterArmFractionOuterBias =
            m_galaxyParameters
            .interArmFractionOuterBias;

        simulationConstants.galaxySpiralSpread =
            m_galaxyParameters.spiralSpread;

        simulationConstants.galaxyCoreFalloff =
            m_galaxyParameters.coreFalloff;

        simulationConstants.galaxySpin =
            m_galaxyParameters.spin;

        simulationConstants.galaxyInnerArmRadius =
            m_galaxyParameters.innerArmRadius;


        simulationConstants.galaxyOuterArmRadius =
            m_galaxyParameters.outerArmRadius;

        simulationConstants.galaxyCoreRadius =
            m_galaxyParameters.coreRadius;

        simulationConstants.galaxyCoreParticleFraction =
            m_galaxyParameters.coreParticleFraction;

        simulationConstants.galaxyCoreHalfThickness =
            m_galaxyParameters.coreHalfThickness;

        simulationConstants.galaxyDiskHalfThickness =
            m_galaxyParameters.diskHalfThickness;

        simulationConstants
            .galaxyRotationCurveRadius = m_galaxyParameters.rotationCurveRadius;
        
        simulationConstants
            .galaxyDifferentialRotationStrength = m_galaxyParameters.differentialRotationStrength;

        simulationConstants.galaxyPadding0 = 0.0f;

        simulationConstants
            .galaxyRadialPerturbationStrength =
                m_galaxyParameters.radialPerturbationStrength;

        simulationConstants
            .galaxyRadialPerturbationFrequencyScale =
                m_galaxyParameters.radialPerturbationFrequencyScale;

        simulationConstants
            .galaxyRadialVelocityDamping =
                m_galaxyParameters.radialVelocityDamping;

        simulationConstants
            .galaxyVerticalOscillationFrequency =
                m_galaxyParameters.verticalOscillationFrequency;

        m_commandList
            ->SetComputeRoot32BitConstants(
                2,
                52,
                &simulationConstants,
                0
            );

        const UINT particleCount =
            m_particleSystem->Count();

        const UINT groupCount =
            (
                particleCount +
                ParticleThreadsPerGroup -
                1
                ) /
            ParticleThreadsPerGroup;


        m_commandList->Dispatch(
            groupCount,
            1,
            1
        );
        
        //
        // Explosion is an impulse,
        // not a persistent simulation mode.
        //
        // The command list already contains
        // this dispatch's root constants,
        // so changing this CPU value now only
        // affects future frames.
        //
        if (
            simulationModeForDispatch ==
            ParticleSimulationMode::Explosion)
        {
            //
            // Explosion is one-shot.
            //
            m_particleSimulationMode =
                ParticleSimulationMode::Orbit;
        }
        else if (
            simulationModeForDispatch ==
            ParticleSimulationMode::Reform)
        {
            //
            // Reform continues for several seconds.
            //
            m_particleReformTimeRemaining =
                std::max(
                    0.0f,
                    m_particleReformTimeRemaining
                    - safeDeltaTime
                );

            if (
                m_particleReformTimeRemaining <=
                0.0f)
            {
                m_particleSimulationMode =
                    ParticleSimulationMode::Orbit;

                core::LogInfo(
                    "Particle reformation complete"
                );
            }
        }

        //
        // We just produced:
        //
        // readBuffer  = old/current state
        // writeBuffer = newly computed state
        //
        // For the NEXT simulation step we want:
        //
        // writeBuffer = readable
        // readBuffer  = writable
        //
        // So reverse their GPU access states.
        //

        D3D12_RESOURCE_BARRIER
            roleTransitions[2]{};


        roleTransitions[0].Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        roleTransitions[0].Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        roleTransitions[0]
            .Transition
            .pResource =
            writeBuffer;

        roleTransitions[0]
            .Transition
            .Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        roleTransitions[0]
            .Transition
            .StateBefore =
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        roleTransitions[0]
            .Transition
            .StateAfter =
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;


        roleTransitions[1].Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        roleTransitions[1].Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        roleTransitions[1]
            .Transition
            .pResource =
            readBuffer;

        roleTransitions[1]
            .Transition
            .Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        roleTransitions[1]
            .Transition
            .StateBefore =
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        roleTransitions[1]
            .Transition
            .StateAfter =
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS;


        m_commandList->ResourceBarrier(
            2,
            roleTransitions
        );
    }

    void D3D12Context::RecordBloomExtraction()
    {
        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(4),
            L"Bloom Extract + Downsample"
        );


        //
        // HDR scene is already PIXEL_SHADER_RESOURCE
        // when this function is called.
        //
        D3D12_RESOURCE_BARRIER
            bloomToRenderTarget{};

        bloomToRenderTarget.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        bloomToRenderTarget.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        bloomToRenderTarget.Transition.pResource =
            m_bloomExtractTexture.Get();

        bloomToRenderTarget.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        bloomToRenderTarget.Transition.StateBefore =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        bloomToRenderTarget.Transition.StateAfter =
            D3D12_RESOURCE_STATE_RENDER_TARGET;


        m_commandList->ResourceBarrier(
            1,
            &bloomToRenderTarget
        );

        const UINT bloomWidth =
            std::max(
                1u,
                m_width / 2u
            );

        const UINT bloomHeight =
            std::max(
                1u,
                m_height / 2u
            );


        D3D12_VIEWPORT
            bloomViewport{};

        bloomViewport.TopLeftX = 0.0f;
        bloomViewport.TopLeftY = 0.0f;

        bloomViewport.Width =
            static_cast<float>(
                bloomWidth
                );

        bloomViewport.Height =
            static_cast<float>(
                bloomHeight
                );

        bloomViewport.MinDepth = 0.0f;
        bloomViewport.MaxDepth = 1.0f;


        D3D12_RECT
            bloomScissor{};

        bloomScissor.left = 0;
        bloomScissor.top = 0;

        bloomScissor.right =
            static_cast<LONG>(
                bloomWidth
                );

        bloomScissor.bottom =
            static_cast<LONG>(
                bloomHeight
                );


        m_commandList->RSSetViewports(
            1,
            &bloomViewport
        );

        m_commandList->RSSetScissorRects(
            1,
            &bloomScissor
        );

        m_commandList->OMSetRenderTargets(
            1,
            &m_bloomExtractRtvHandle,
            FALSE,
            nullptr
        );


        const float clearColor[4]
        {
            0.0f,
            0.0f,
            0.0f,
            1.0f
        };

        m_commandList->ClearRenderTargetView(
            m_bloomExtractRtvHandle,
            clearColor,
            0,
            nullptr
        );

        m_commandList->SetPipelineState(
            m_bloomExtractPipeline
            ->PipelineState()
        );

        m_commandList->SetGraphicsRootSignature(
            m_bloomExtractPipeline
            ->RootSignature()
        );


        m_commandList
            ->SetGraphicsRootDescriptorTable(
                0,
                m_hdrSceneSrv.gpu
            );


        m_commandList
            ->SetGraphicsRoot32BitConstants(
                1,
                1,
                &m_bloomThreshold,
                0
            );


        m_commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );


        m_commandList->DrawInstanced(
            3,
            1,
            0,
            0
        );

        D3D12_RESOURCE_BARRIER
            bloomToShaderResource{};

        bloomToShaderResource.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        bloomToShaderResource.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        bloomToShaderResource.Transition.pResource =
            m_bloomExtractTexture.Get();

        bloomToShaderResource.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        bloomToShaderResource.Transition.StateBefore =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        bloomToShaderResource.Transition.StateAfter =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;


        m_commandList->ResourceBarrier(
            1,
            &bloomToShaderResource
        );


        //
        // Restore the full-resolution viewport because
        // presentation comes next.
        //
        m_commandList->RSSetViewports(
            1,
            &m_viewport
        );

        m_commandList->RSSetScissorRects(
            1,
            &m_scissorRect
        );
    }

    void D3D12Context::RecordBloomBlur()
    {
        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(5),
            L"Bloom Gaussian Blur"
        );


        const UINT bloomWidth =
            std::max(
                1u,
                m_width / 2u
            );

        const UINT bloomHeight =
            std::max(
                1u,
                m_height / 2u
            );


        D3D12_VIEWPORT
            bloomViewport{};

        bloomViewport.TopLeftX = 0.0f;
        bloomViewport.TopLeftY = 0.0f;

        bloomViewport.Width =
            static_cast<float>(
                bloomWidth
                );

        bloomViewport.Height =
            static_cast<float>(
                bloomHeight
                );

        bloomViewport.MinDepth = 0.0f;
        bloomViewport.MaxDepth = 1.0f;


        D3D12_RECT
            bloomScissor{};

        bloomScissor.left = 0;
        bloomScissor.top = 0;

        bloomScissor.right =
            static_cast<LONG>(
                bloomWidth
                );

        bloomScissor.bottom =
            static_cast<LONG>(
                bloomHeight
                );


        m_commandList->RSSetViewports(
            1,
            &bloomViewport
        );

        m_commandList->RSSetScissorRects(
            1,
            &bloomScissor
        );


        m_commandList->SetPipelineState(
            m_bloomBlurPipeline
            ->PipelineState()
        );

        m_commandList->SetGraphicsRootSignature(
            m_bloomBlurPipeline
            ->RootSignature()
        );

        m_commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );

        D3D12_RESOURCE_BARRIER
            tempToRenderTarget{};

        tempToRenderTarget.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        tempToRenderTarget.Transition.pResource =
            m_bloomBlurTempTexture.Get();

        tempToRenderTarget.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        tempToRenderTarget.Transition.StateBefore =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        tempToRenderTarget.Transition.StateAfter =
            D3D12_RESOURCE_STATE_RENDER_TARGET;


        m_commandList->ResourceBarrier(
            1,
            &tempToRenderTarget
        );

        m_commandList->OMSetRenderTargets(
            1,
            &m_bloomBlurTempRtvHandle,
            FALSE,
            nullptr
        );

        m_commandList
            ->SetGraphicsRootDescriptorTable(
                0,
                m_bloomExtractSrv.gpu
            );

        const float horizontalConstants[4]
        {
            1.0f /
            static_cast<float>(
                bloomWidth
            ),

            1.0f /
            static_cast<float>(
                bloomHeight
            ),

            1.0f,
            0.0f
        };

        m_commandList
            ->SetGraphicsRoot32BitConstants(
                1,
                4,
                horizontalConstants,
                0
            );

        m_commandList->DrawInstanced(
            3,
            1,
            0,
            0
        );

        D3D12_RESOURCE_BARRIER
            verticalPassBarriers[2]{};


        verticalPassBarriers[0].Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        verticalPassBarriers[0]
            .Transition.pResource =
            m_bloomBlurTempTexture.Get();

        verticalPassBarriers[0]
            .Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        verticalPassBarriers[0]
            .Transition.StateBefore =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        verticalPassBarriers[0]
            .Transition.StateAfter =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;


        verticalPassBarriers[1].Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        verticalPassBarriers[1]
            .Transition.pResource =
            m_bloomExtractTexture.Get();

        verticalPassBarriers[1]
            .Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        verticalPassBarriers[1]
            .Transition.StateBefore =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        verticalPassBarriers[1]
            .Transition.StateAfter =
            D3D12_RESOURCE_STATE_RENDER_TARGET;


        m_commandList->ResourceBarrier(
            _countof(verticalPassBarriers),
            verticalPassBarriers
        );

        m_commandList->OMSetRenderTargets(
            1,
            &m_bloomExtractRtvHandle,
            FALSE,
            nullptr
        );

        m_commandList
            ->SetGraphicsRootDescriptorTable(
                0,
                m_bloomBlurTempSrv.gpu
            );

        const float verticalConstants[4]
        {
            1.0f /
            static_cast<float>(
                bloomWidth
            ),

            1.0f /
            static_cast<float>(
                bloomHeight
            ),

            0.0f,
            1.0f
        };

        m_commandList
            ->SetGraphicsRoot32BitConstants(
                1,
                4,
                verticalConstants,
                0
            );

        m_commandList->DrawInstanced(
            3,
            1,
            0,
            0
        );

        D3D12_RESOURCE_BARRIER
            bloomToShaderResource{};

        bloomToShaderResource.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        bloomToShaderResource.Transition.pResource =
            m_bloomExtractTexture.Get();

        bloomToShaderResource.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        bloomToShaderResource.Transition.StateBefore =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        bloomToShaderResource.Transition.StateAfter =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;


        m_commandList->ResourceBarrier(
            1,
            &bloomToShaderResource
        );

        m_commandList->RSSetViewports(
            1,
            &m_viewport
        );

        m_commandList->RSSetScissorRects(
            1,
            &m_scissorRect
        );
    }

    void D3D12Context::RecordParticleDebugDraw(
        gf::graphics::frame::FrameResource&
        frameResource,
        const gf::scene::Camera& camera,
        std::uint32_t constantSlot)
    {
        if (m_particleSystem == nullptr)
        {
            throw std::runtime_error(
                "Particle debug draw requires "
                "a ParticleSystem"
            );
        }

        if (m_particleDebugPipeline == nullptr)
        {
            throw std::runtime_error(
                "Particle debug draw requires "
                "a graphics pipeline"
            );
        }

        if (
            constantSlot >=
            gf::graphics::frame::FrameResource::
            MaxSceneObjects)
        {
            throw std::runtime_error(
                "Particle debug draw exceeds "
                "frame constant capacity"
            );
        }


        //
        // IMPORTANT:
        //
        // RecordParticleSimulation() has already run
        // earlier in this SAME command list.
        //
        // The logical WRITE buffer therefore contains
        // this frame's newly simulated state.
        //
        // It has also already been transitioned:
        //
        // UNORDERED_ACCESS
        //      ->
        // NON_PIXEL_SHADER_RESOURCE
        //
        const std::uint32_t
            currentParticleBufferIndex =
            m_particleSimulationEnabled
            ? m_particleSystem
                ->WriteBufferIndex()
            : m_particleSystem
                ->ReadBufferIndex();


        const DirectX::XMMATRIX
            viewProjection =
            camera.ViewMatrix() *
            camera.ProjectionMatrix();


        gf::graphics::frame::SceneConstants
            particleConstants{};

        DirectX::XMStoreFloat4x4(
            &particleConstants
            .modelViewProjection,
            viewProjection
        );


        //
        // Deliberately bright HDR debug color.
        //
        // This is NOT the final particle art.
        //
        particleConstants.colorTint =
            DirectX::XMFLOAT4{
                0.20f,
                2.5f,
                8.0f,
                1.0f
        };


        frameResource.WriteSceneConstants(
            constantSlot,
            particleConstants
        );


        m_commandList->SetPipelineState(
            m_particleDebugPipeline
            ->PipelineState()
        );

        m_commandList->SetGraphicsRootSignature(
            m_particleDebugPipeline
            ->RootSignature()
        );


        m_commandList
            ->SetGraphicsRootConstantBufferView(
                0,
                frameResource
                .SceneConstantBufferAddress(
                    constantSlot
                )
            );


        m_commandList
            ->SetGraphicsRootDescriptorTable(
                1,
                m_particleBufferViews[
                    currentParticleBufferIndex
                ].srv.gpu
            );


        m_commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_POINTLIST
        );


        const UINT debugDrawCount =
            std::min(
                m_particleDebugDrawCount,
                m_particleSystem->Count()
            );

        m_commandList->DrawInstanced(
            debugDrawCount,
            1,
            0,
            0
        );
    }


    void D3D12Context::
        RecordParticleBillboardDraw(
            gf::graphics::frame::FrameResource&
            frameResource,
            const gf::scene::Camera& camera,
            std::uint32_t constantSlot,
            float elapsedSeconds)
    {
        if (m_particleSystem == nullptr)
        {
            throw std::runtime_error(
                "Particle billboard draw requires "
                "a ParticleSystem"
            );
        }

        if (m_particleBillboardPipeline == nullptr)
        {
            throw std::runtime_error(
                "Particle billboard draw requires "
                "a graphics pipeline"
            );
        }

        if (
            constantSlot >=
            gf::graphics::frame::FrameResource::
            MaxSceneObjects)
        {
            throw std::runtime_error(
                "Particle billboard draw exceeds "
                "frame constant capacity"
            );
        }

        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(2),
            L"Particle Billboards"
        );

        //
        // If simulation ran earlier in this command list,
        // its logical WRITE resource now contains this
        // frame's newly produced state.
        //
        // That resource has already transitioned:
        //
        // UNORDERED_ACCESS
        //      ->
        // NON_PIXEL_SHADER_RESOURCE
        //
        // If simulation is paused, there was no new write,
        // so the current logical READ resource remains
        // the state that should be rendered.
        //
        const std::uint32_t
            currentParticleBufferIndex =
            m_particleSimulationEnabled
            ? m_particleSystem
            ->WriteBufferIndex()
            : m_particleSystem
            ->ReadBufferIndex();


        const DirectX::XMMATRIX
            viewProjection =
            camera.ViewMatrix() *
            camera.ProjectionMatrix();


        gf::graphics::frame::
            ParticleRenderConstants
            particleConstants{};


        DirectX::XMStoreFloat4x4(
            &particleConstants.viewProjection,
            viewProjection
        );


        DirectX::XMStoreFloat3(
            &particleConstants.cameraRight,
            camera.RightVector()
        );


        DirectX::XMStoreFloat3(
            &particleConstants.cameraUp,
            camera.UpVector()
        );


        particleConstants.particleHalfSize =
            ParticleBillboardHalfSize;


        //
        // Per-particle colour and luminance are
        // generated in the billboard shader from
        // galaxy radius and deterministic seed data.
        //
        // This remains a global tint / intensity scale.
        //
        particleConstants.color =
            DirectX::XMFLOAT4{
                1.0f,
                1.0f,
                1.0f,
                1.00f
        };

        particleConstants.coreParameters =
            DirectX::XMFLOAT4{
                m_galaxyParameters.coreRadius,
                m_galaxyParameters.coreParticleFraction,
                m_galaxyParameters.coreParticleSizeMultiplier,
                m_galaxyParameters.coreParticleIntensity
        };

        particleConstants.coreColor =
            DirectX::XMFLOAT4{
                m_galaxyParameters.coreColor[0],
                m_galaxyParameters.coreColor[1],
                m_galaxyParameters.coreColor[2],
                0.0f
        };


        particleConstants.outerArmColor =
            DirectX::XMFLOAT4{
                m_galaxyParameters.outerArmColor[0],
                m_galaxyParameters.outerArmColor[1],
                m_galaxyParameters.outerArmColor[2],
                0.0f
        };


        particleConstants.colorParameters =
            DirectX::XMFLOAT4{
                m_galaxyParameters.outerArmRadius,
                m_galaxyParameters.colorGradientExponent,
                m_galaxyParameters.colorJitterStrength,
                0.0f
        };

        particleConstants.toneParameters =
            DirectX::XMFLOAT4{
                m_galaxyParameters.outerArmIntensity,
                m_galaxyParameters.brightnessFalloffExponent,
                m_galaxyParameters.stellarIntensityJitter,
                m_galaxyParameters.starHdrIntensity
        };

        particleConstants.elapsedSeconds =
            elapsedSeconds;

        particleConstants.starParameters =
            DirectX::XMFLOAT4{
                m_galaxyParameters.starParticleFraction,
                m_galaxyParameters.starRotationSpeed,
                m_galaxyParameters.starPulseAmplitude,
                m_galaxyParameters.starPulseFrequency
        };

        frameResource
            .WriteParticleRenderConstants(
                constantSlot,
                particleConstants
            );


        m_commandList->SetPipelineState(
            m_particleBillboardPipeline
            ->PipelineState(
                m_particleDepthTestingEnabled
            )
        );


        m_commandList->SetGraphicsRootSignature(
            m_particleBillboardPipeline
            ->RootSignature()
        );


        //
        // Root parameter 0:
        // b0 / ParticleRenderConstants.
        //
        m_commandList
            ->SetGraphicsRootConstantBufferView(
                0,
                frameResource
                .SceneConstantBufferAddress(
                    constantSlot
                )
            );


        //
        // Root parameter 1:
        // t0 / current GPU ParticleState SRV.
        //
        m_commandList
            ->SetGraphicsRootDescriptorTable(
                1,
                m_particleBufferViews[
                    currentParticleBufferIndex
                ].srv.gpu
            );


        m_commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );


        //
        // Reuse the M8 count control temporarily.
        //
        // P cycles:
        // low -> medium -> full.
        //
        const UINT particleDrawCount =
            std::min(
                m_particleDebugDrawCount,
                m_particleSystem->Count()
            );


        //
        // 6 vertices PER INSTANCE.
        //
        // One instance = one particle.
        //
        m_commandList->DrawInstanced(
            ParticleBillboardVertexCount,
            particleDrawCount,
            0,
            0
        );
    }

    void D3D12Context::RecordDebrisCulling(
        const gf::scene::Camera& camera)
    {
        if (
            m_debrisCullingPipeline == nullptr ||
            m_debrisField == nullptr ||
            m_debrisVisibilityFlags == nullptr ||
            m_debrisVisibleInstanceIndices == nullptr ||
            m_debrisVisibleCount == nullptr ||
            m_debrisVisibleCountClearHeap == nullptr)
        {
            throw std::runtime_error(
                "Debris culling resources are incomplete"
            );
        }

        if (m_debrisDrawCount == 0)
        {
            return;
        }

        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(4),
            m_debrisCullingDebugEnabled
            ? L"Debris GPU Cull + Compact [Debug Inset]"
            : L"Debris GPU Cull + Compact"
        );

        const auto transitionBarrier =
            [](
                ID3D12Resource* resource,
                D3D12_RESOURCE_STATES before,
                D3D12_RESOURCE_STATES after)
            {
                D3D12_RESOURCE_BARRIER barrier{};

                barrier.Type =
                    D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

                barrier.Transition.pResource = resource;
                barrier.Transition.StateBefore = before;
                barrier.Transition.StateAfter = after;

                barrier.Transition.Subresource =
                    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                return barrier;
            };

        D3D12_RESOURCE_BARRIER toUavBarriers[]
        {
            transitionBarrier(
                m_debrisVisibilityFlags.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            ),
            transitionBarrier(
                m_debrisVisibleInstanceIndices.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            ),
            transitionBarrier(
                m_debrisVisibleCount.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            )
        };

        m_commandList->ResourceBarrier(
            _countof(toUavBarriers),
            toUavBarriers
        );

        const UINT clearValues[4]
        {
            0,
            0,
            0,
            0
        };

        m_commandList->ClearUnorderedAccessViewUint(
            m_debrisVisibleCountUav.gpu,
            m_debrisVisibleCountClearHeap
            ->GetCPUDescriptorHandleForHeapStart(),
            m_debrisVisibleCount.Get(),
            clearValues,
            0,
            nullptr
        );

        D3D12_RESOURCE_BARRIER
            counterClearBarrier{};

        counterClearBarrier.Type =
            D3D12_RESOURCE_BARRIER_TYPE_UAV;

        counterClearBarrier.UAV.pResource =
            m_debrisVisibleCount.Get();

        m_commandList->ResourceBarrier(
            1,
            &counterClearBarrier
        );

        m_commandList->SetPipelineState(
            m_debrisCullingPipeline
            ->PipelineState()
        );

        m_commandList->SetComputeRootSignature(
            m_debrisCullingPipeline
            ->RootSignature()
        );

        m_commandList->SetComputeRootDescriptorTable(
            0,
            m_debrisInstancesSrv.gpu
        );

        m_commandList->SetComputeRootDescriptorTable(
            1,
            m_debrisVisibilityFlagsUav.gpu
        );

        m_commandList->SetComputeRootDescriptorTable(
            2,
            m_debrisVisibleInstanceIndicesUav.gpu
        );

        m_commandList->SetComputeRootDescriptorTable(
            3,
            m_debrisVisibleCountUav.gpu
        );

        const DirectX::XMMATRIX
            viewProjection =
            camera.ViewMatrix() *
            camera.ProjectionMatrix();

        DebrisCullingRootConstants
            constants{};

        constants.frustumPlanes =
            ExtractFrustumPlanes(
                viewProjection
            );

        constants.animationSeconds =
            m_debrisAnimationTime;

        constants.instanceCount =
            m_debrisDrawCount;

        constants.frustumInset =
            m_debrisCullingDebugEnabled
            ? 3.0f
            : 0.0f;


        m_commandList->SetComputeRoot32BitConstants(
            4,
            sizeof(constants) /
            sizeof(std::uint32_t),
            &constants,
            0
        );


        const UINT dispatchGroupCount =
            (
                m_debrisDrawCount +
                DebrisCullingThreadsPerGroup -
                1
                ) /
            DebrisCullingThreadsPerGroup;

        m_commandList->Dispatch(
            dispatchGroupCount,
            1,
            1
        );


        D3D12_RESOURCE_BARRIER uavBarriers[3]{};

        ID3D12Resource* uavResources[3]
        {
            m_debrisVisibilityFlags.Get(),
            m_debrisVisibleInstanceIndices.Get(),
            m_debrisVisibleCount.Get()
        };

        for (UINT index = 0; index < 3; ++index)
        {
            uavBarriers[index].Type =
                D3D12_RESOURCE_BARRIER_TYPE_UAV;

            uavBarriers[index].UAV.pResource =
                uavResources[index];
        }

        m_commandList->ResourceBarrier(
            _countof(uavBarriers),
            uavBarriers
        );

        const bool useIndirect =
            m_debrisIndirectEnabled &&
            !m_debrisCullingDebugEnabled;

        if (!useIndirect)
        {
            D3D12_RESOURCE_BARRIER
                toGraphicsBarriers[]
            {
                transitionBarrier(
                    m_debrisVisibilityFlags.Get(),
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                ),

                transitionBarrier(
                    m_debrisVisibleInstanceIndices.Get(),
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                ),

                transitionBarrier(
                    m_debrisVisibleCount.Get(),
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                )
            };

            m_commandList->ResourceBarrier(
                _countof(toGraphicsBarriers),
                toGraphicsBarriers
            );

            return;
        }

        D3D12_RESOURCE_BARRIER
            toIndirectPreparation[]
        {
            transitionBarrier(
                m_debrisVisibilityFlags.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            ),

            transitionBarrier(
                m_debrisVisibleInstanceIndices.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            ),

            transitionBarrier(
                m_debrisVisibleCount.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_COPY_SOURCE
            ),

            transitionBarrier(
                m_debrisIndirectArguments.Get(),
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                D3D12_RESOURCE_STATE_COPY_DEST
            )
        };

        m_commandList->ResourceBarrier(
            _countof(toIndirectPreparation),
            toIndirectPreparation
        );

        constexpr UINT64 instanceCountOffset =
            offsetof(
                D3D12_DRAW_INDEXED_ARGUMENTS,
                InstanceCount
            );

        m_commandList->CopyBufferRegion(
            m_debrisIndirectArguments.Get(),
            instanceCountOffset,
            m_debrisVisibleCount.Get(),
            0,
            sizeof(UINT)
        );

        D3D12_RESOURCE_BARRIER
            indirectReadyBarriers[]
        {
            transitionBarrier(
                m_debrisVisibleCount.Get(),
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            ),

            transitionBarrier(
                m_debrisIndirectArguments.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
            )
        };

        m_commandList->ResourceBarrier(
            _countof(indirectReadyBarriers),
            indirectReadyBarriers
        );
    }

    void D3D12Context::RecordFrame(
        float elapsedSeconds,
        float deltaSeconds,
        gf::graphics::frame::FrameResource&
        frameResource,
        const gf::scene::Camera& camera
    )
    {
        if (m_debrisAnimationEnabled)
        {
            m_debrisAnimationTime +=
                std::clamp(
                    deltaSeconds,
                    0.0f,
                    0.1f
                );
        }

        m_commandList->RSSetViewports(
            1,
            &m_viewport
        );

        m_commandList->RSSetScissorRects(
            1,
            &m_scissorRect
        );

        ID3D12DescriptorHeap* descriptorHeaps[]
        {
            m_srvHeap.Get()
        };

        m_commandList->SetDescriptorHeaps(
            1,
            descriptorHeaps
        );

        // PASS 1:
        // Particle simulation.
        RecordParticleSimulation(
            deltaSeconds
        );

        // PASS 2:
        // No longer doing separate debris workload.
        // Kept for future re-introduction in the
        // galaxy-directioned debris system.

        // PASS 3:
        // Restore the normal scene graphics pipeline.
        ID3D12PipelineState* scenePipelineState =
            m_basicPipeline->PipelineState(
                m_depthTestingEnabled,
                gf::graphics::pipeline::
                PipelineKind::Surface
        );


        m_commandList->SetPipelineState(
            scenePipelineState
        );

        m_commandList->SetGraphicsRootSignature(
            m_basicPipeline->RootSignature()
        );

        D3D12_RESOURCE_BARRIER
            hdrToRenderTarget{};

        hdrToRenderTarget.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        hdrToRenderTarget.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        hdrToRenderTarget.Transition.pResource =
            m_hdrSceneColor.Get();

        hdrToRenderTarget.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        hdrToRenderTarget.Transition.StateBefore =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        hdrToRenderTarget.Transition.StateAfter =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        m_commandList->ResourceBarrier(
            1,
            &hdrToRenderTarget
        );

        const D3D12_CPU_DESCRIPTOR_HANDLE sceneRtv = m_hdrSceneRtvHandle;

        m_commandList->OMSetRenderTargets(
            1,
            &sceneRtv,
            FALSE,
            &m_dsvHandle
        );

        const float clearColor[4]{
            0.f,
            0.f,
            0.f,
            1.0f
        };

        m_commandList->ClearRenderTargetView(
            sceneRtv,
            clearColor,
            0,
            nullptr
        );

        m_commandList->ClearDepthStencilView(
            m_dsvHandle,
            D3D12_CLEAR_FLAG_DEPTH,
            1.0f,
            0,
            0,
            nullptr
        );

        m_commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );

        ID3D12PipelineState* activePipelineState =
            scenePipelineState;

        std::uint32_t objectIndex = 0;

        for (
            const gf::scene::ReactorPart& part :
            m_reactorScene.Parts())
        {
            if (
                objectIndex >=
                gf::graphics::frame::FrameResource
                ::MaxSceneObjects)
            {
                throw std::runtime_error(
                    "Reactor exceeds frame object capacity"
                );
            }

            const DirectX::XMMATRIX model =
                gf::scene::BuildReactorPartModel(
                    part,
                    elapsedSeconds
                );

            const gf::graphics::pipeline::PipelineKind
                requiredPipelineKind =
                part.pipeline ==
                gf::scene::ReactorPipelineKind::Emissive
                ? gf::graphics::pipeline::PipelineKind::Emissive
                : gf::graphics::pipeline::PipelineKind::Surface;

            ID3D12PipelineState* requiredPipelineState =
                m_basicPipeline->PipelineState(
                    m_depthTestingEnabled,
                    requiredPipelineKind
                );

            if (
                requiredPipelineState !=
                activePipelineState)
            {
                m_commandList->SetPipelineState(
                    requiredPipelineState
                );

                activePipelineState =
                    requiredPipelineState;
            }

            DrawMesh(
                MeshFor(part.mesh),
                frameResource,
                camera,
                model,
                MaterialFor(part.material),
                elapsedSeconds,
                objectIndex
            );

            ++objectIndex;
        }

        // PASS 3:
        // GPU particle billboard rendering.
        //
        // HDR is still RENDER_TARGET.
        //
        // Opaque reactor geometry has already
        // populated the depth buffer.
        //
        // The current particle simulation output
        // is NON_PIXEL_SHADER_RESOURCE and is
        // consumed directly by the billboard VS.
        //
        if (m_particleRenderingEnabled)
        {
            RecordParticleBillboardDraw(
                frameResource,
                camera,
                objectIndex,
                elapsedSeconds
            );

            /*RecordParticleDebugDraw(
                frameResource,
                camera,
                objectIndex
            );*/
            
            ++objectIndex;
        }

        D3D12_RESOURCE_BARRIER
            hdrToShaderResource{};

        hdrToShaderResource.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        hdrToShaderResource.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        hdrToShaderResource.Transition.pResource =
            m_hdrSceneColor.Get();

        hdrToShaderResource.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        hdrToShaderResource.Transition.StateBefore =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        hdrToShaderResource.Transition.StateAfter =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        m_commandList->ResourceBarrier(
            1,
            &hdrToShaderResource
        );

        //
        // PASS 4:
        // Bloom extraction + blur (if enabled).
        //
        if (m_bloomEnabled)
        {
            RecordBloomExtraction();
            RecordBloomBlur();
        }

        D3D12_RESOURCE_BARRIER
            backBufferToRenderTarget{};

        backBufferToRenderTarget.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        backBufferToRenderTarget.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        backBufferToRenderTarget
            .Transition
            .pResource =
            m_backBuffers[
                m_backBufferIndex
            ].Get();

        backBufferToRenderTarget
            .Transition
            .Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        backBufferToRenderTarget
            .Transition
            .StateBefore =
            D3D12_RESOURCE_STATE_PRESENT;

        backBufferToRenderTarget
            .Transition
            .StateAfter =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        m_commandList->ResourceBarrier(
            1,
            &backBufferToRenderTarget
        );

        const D3D12_CPU_DESCRIPTOR_HANDLE
            backBufferRtv =
            m_rtvHandles[
                m_backBufferIndex
            ];

        m_commandList->OMSetRenderTargets(
            1,
            &backBufferRtv,
            FALSE,
            nullptr
        );

        PIXScopedEvent(
            m_commandList.Get(),
            PIX_COLOR_INDEX(6),
            m_bloomEnabled
            ? L"HDR Presentation [Bloom]"
            : L"HDR Presentation [No Bloom]"
        );

        m_commandList->SetPipelineState(
            m_presentationPipeline
            ->PipelineState()
        );

        m_commandList->SetGraphicsRootSignature(
            m_presentationPipeline
            ->RootSignature()
        );

        m_commandList
            ->SetGraphicsRootDescriptorTable(
                0,
                m_hdrSceneSrv.gpu
            );

        //
        // t1 = filtered bloom.
        //
        m_commandList
            ->SetGraphicsRootDescriptorTable(
                1,
                m_bloomExtractSrv.gpu
            );

        const float effectiveBloomIntensity =
            m_bloomEnabled
            ? m_bloomIntensity
            : 0.0f;

        const float presentationConstants[2]
        {
            m_exposure,
            effectiveBloomIntensity
        };

        m_commandList
            ->SetGraphicsRoot32BitConstants(
                2,
                2,
                presentationConstants,
                0
            );

        m_commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );

        m_commandList->DrawInstanced(
            3,
            1,
            0,
            0
        );

        D3D12_RESOURCE_BARRIER
            backBufferToPresent{};

        backBufferToPresent.Type =
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        backBufferToPresent.Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        backBufferToPresent
            .Transition
            .pResource =
            m_backBuffers[
                m_backBufferIndex
            ].Get();

        backBufferToPresent
            .Transition
            .Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        backBufferToPresent
            .Transition
            .StateBefore =
            D3D12_RESOURCE_STATE_RENDER_TARGET;

        backBufferToPresent
            .Transition
            .StateAfter =
            D3D12_RESOURCE_STATE_PRESENT;

        m_commandList->ResourceBarrier(
            1,
            &backBufferToPresent
        );
    }

    void D3D12Context::EndFrame(
        gf::graphics::frame::FrameResource&
        frameResource)
    {
        core::ThrowIfFailed(
            m_commandList->Close(),
            "ID3D12GraphicsCommandList::"
            "Close failed"
        );

        ID3D12CommandList* commandLists[]{
        m_commandList.Get()
        };

        m_commandQueue->ExecuteCommandLists(
            1,
            commandLists
        );

        // The submitted simulation wrote the current
        // write buffer and recorded transitions that
        // reverse its role for the next step.
        //
        // No GPU wait is needed here.
        if (m_particleSimulationEnabled)
        {
            m_particleSystem->SwapBuffers();
        }

        core::ThrowIfFailed(
            m_swapChain->Present(1, 0),
            "IDXGISwapChain::Present failed"
        );

        const UINT64 submittedFenceValue =
            m_nextFenceValue;

        ++m_nextFenceValue;

        core::ThrowIfFailed(
            m_commandQueue->Signal(
                m_fence.Get(),
                submittedFenceValue
            ),
            "ID3D12CommandQueue::Signal failed"
        );

        frameResource.SetFenceValue(submittedFenceValue);

        m_lastSubmittedFenceValue = submittedFenceValue;

        ++m_submittedFrameCount;

        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        m_frameResourceIndex = (m_frameResourceIndex + 1) % FrameResourceCount;
    }

    void D3D12Context::Render(
        float elapsedSeconds,
        float deltaSeconds,
        const gf::scene::Camera& camera)
    {
        gf::graphics::frame::FrameResource&
            frameResource = BeginFrame();

        RecordFrame(
            elapsedSeconds,
            deltaSeconds,
            frameResource,
            camera
        );

        EndFrame(frameResource);
    }

    void D3D12Context::Resize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        if (width == m_width &&
            height == m_height)
        {
            return;
        }

        FlushGpu();

        m_depthBuffer.Reset();
        m_hdrSceneColor.Reset();
        m_bloomExtractTexture.Reset();
        m_bloomBlurTempTexture.Reset();

        for (auto& backBuffer : m_backBuffers)
        {
            backBuffer.Reset();
        }

        core::ThrowIfFailed(
            m_swapChain->ResizeBuffers(
                SwapChainBufferCount,
                width,
                height,
                BackBufferFormat,
                0
            ),
            "IDXGISwapChain::ResizeBuffers failed"
        );

        m_width = width;
        m_height = height;

        UpdateViewportAndScissor();

        m_backBufferIndex =
            m_swapChain->GetCurrentBackBufferIndex();

        CreateBackBuffers();
        CreateDepthBuffer();
        CreateHdrSceneTarget();
        CreateBloomExtractTarget();
        CreateBloomBlurTempTarget();

        for (auto& frameResource : m_frameResources)
        {
            frameResource.SetFenceValue(0);
        }

        m_frameResourceIndex = 0;

        core::LogInfo("Swap chain resized");
    }

    void D3D12Context::ToggleDepthTesting()
    {
        m_depthTestingEnabled =
            !m_depthTestingEnabled;

        core::LogInfo(
            m_depthTestingEnabled
            ? "Depth testing enabled"
            : "Depth testing disabled"
        );
    }

    void D3D12Context::AdjustCoreParticleIntensity(
        float delta
    ) noexcept
    {
        m_galaxyParameters
            .coreParticleIntensity =
            std::clamp(
                m_galaxyParameters
                .coreParticleIntensity +
                delta,
                1.0f,
                6.0f
            );
    }

    void D3D12Context::AdjustExposure(
        float delta) noexcept
    {
        m_exposure =
            std::clamp(
                m_exposure + delta,
                0.10f,
                4.0f
            );
    }

    void D3D12Context::ToggleBloom()
    {
        m_bloomEnabled =
            !m_bloomEnabled;

        core::LogInfo(
            m_bloomEnabled
            ? "Bloom enabled"
            : "Bloom disabled"
        );
    }

    void D3D12Context::IncreaseBloomIntensity()
    {
        m_bloomIntensity =
            std::min(
                m_bloomIntensity + 0.05f,
                2.0f
            );

        core::LogInfo(
            std::format(
                "Bloom intensity: {:.2f}",
                m_bloomIntensity
            )
        );
    }

    void D3D12Context::DecreaseBloomIntensity()
    {
        m_bloomIntensity =
            std::max(
                m_bloomIntensity - 0.05f,
                0.0f
            );

        core::LogInfo(
            std::format(
                "Bloom intensity: {:.2f}",
                m_bloomIntensity
            )
        );
    }

    void D3D12Context::IncreaseBloomThreshold()
    {
        m_bloomThreshold =
            std::min(
                m_bloomThreshold + 0.1f,
                4.0f
            );

        core::LogInfo(
            std::format(
                "Bloom threshold: {:.2f}",
                m_bloomThreshold
            )
        );
    }

    void D3D12Context::DecreaseBloomThreshold()
    {
        m_bloomThreshold =
            std::max(
                m_bloomThreshold - 0.1f,
                0.25f
            );

        core::LogInfo(
            std::format(
                "Bloom threshold: {:.2f}",
                m_bloomThreshold
            )
        );
    }

    void D3D12Context::IncreaseExposure()
    {
        m_exposure =
            std::min(
                m_exposure + 0.1f,
                4.0f
            );

        core::LogInfo(
            std::format(
                "Exposure: {:.2f}",
                m_exposure
            )
        );
    }

    void D3D12Context::DecreaseExposure()
    {
        m_exposure =
            std::max(
                m_exposure - 0.1f,
                0.1f
            );

        core::LogInfo(
            std::format(
                "Exposure: {:.2f}",
                m_exposure
            )
        );
    }

    void D3D12Context::ToggleParticleSimulation()
    {
        m_particleSimulationEnabled =
            !m_particleSimulationEnabled;

        core::LogInfo(
            m_particleSimulationEnabled
            ? "Particle simulation resumed"
            : "Particle simulation paused"
        );
    }

    void D3D12Context::
        ToggleParticleRendering()
    {
        m_particleRenderingEnabled =
            !m_particleRenderingEnabled;

        core::LogInfo(
            m_particleRenderingEnabled
            ? "Particle billboard rendering enabled"
            : "Particle billboard rendering disabled"
        );
    }

    void D3D12Context::
        ToggleParticleDepthTesting()
    {
        m_particleDepthTestingEnabled =
            !m_particleDepthTestingEnabled;

        core::LogInfo(
            m_particleDepthTestingEnabled
            ? "Particle depth testing enabled"
            : "Particle depth testing disabled"
        );
    }

    void D3D12Context::
        ToggleInteractiveGravitySource()
    {
        m_interactiveGravityEnabled =
            !m_interactiveGravityEnabled;

        core::LogInfo(
            m_interactiveGravityEnabled
            ? "Interactive gravity source enabled"
            : "Interactive gravity source disabled"
        );
    }

    void D3D12Context::
        MoveInteractiveGravitySource(
            float deltaX,
            float deltaY,
            float deltaZ
        ) noexcept
    {
        constexpr float sourceBounds =
            4.0f;

        auto& source =
            m_interactiveGravitySources[
                m_selectedInteractiveGravitySource
            ];

        source.position.x =
            std::clamp(
                source.position.x + deltaX,
                -sourceBounds,
                sourceBounds
            );

        source.position.y =
            std::clamp(
                source.position.y + deltaY,
                -sourceBounds,
                sourceBounds
            );

        source.position.z =
            std::clamp(
                source.position.z + deltaZ,
                -sourceBounds,
                sourceBounds
            );
    }

    void D3D12Context::
        AdjustGalaxySpin(
            float delta) noexcept
    {
        m_galaxyParameters.spin =
            std::clamp(
                m_galaxyParameters.spin +
                delta,
                0.0f,
                1.25f
            );

        core::LogInfo(
            std::format(
                "Galaxy spin: {:.2f} rad/s",
                m_galaxyParameters.spin
            )
        );
    }


    void D3D12Context::
        AdjustDifferentialRotationStrength(
            float delta) noexcept
    {
        m_galaxyParameters
            .differentialRotationStrength =
            std::clamp(
                m_galaxyParameters
                    .differentialRotationStrength +
                delta,
                0.0f,
                1.0f
            );

        core::LogInfo(
            std::format(
                "Galaxy differential rotation strength: {:.2f}",
                m_galaxyParameters
                    .differentialRotationStrength
            )
        );
    }

    void D3D12Context::
        AdjustSpiralStrength(
            float delta) noexcept
    {
        m_galaxyParameters
            .spiralSpread =
            std::clamp(
                m_galaxyParameters
                    .spiralSpread +
                delta,
                0.0f,
                1.8f
            );

        core::LogInfo(
            std::format(
                "Galaxy spiral strength: {:.2f}",
                m_galaxyParameters
                    .spiralSpread
            )
        );
    }

    void D3D12Context::
        AdjustArmConcentration(
            float delta) noexcept
    {
        m_galaxyParameters
            .armConcentration =
            std::clamp(
                m_galaxyParameters
                    .armConcentration +
                delta,
                1.0f,
                24.0f
            );

        core::LogInfo(
            std::format(
                "Galaxy arm concentration: {:.2f}",
                m_galaxyParameters
                    .armConcentration
            )
        );
    }

    void D3D12Context::
        AdjustParticleDensity(
            float delta) noexcept
    {
        if (m_particleSystem == nullptr)
        {
            return;
        }

        m_particleDensityMultiplier =
            std::clamp(
                m_particleDensityMultiplier +
                delta,
                0.05f,
                1.0f
            );

        const std::uint32_t fullCount =
            m_particleSystem->Count();

        m_particleDebugDrawCount =
            std::max(
                1u,
                static_cast<std::uint32_t>(
                    std::round(
                        m_particleDensityMultiplier *
                        static_cast<float>(fullCount)
                    )
                )
            );

        core::LogInfo(
            std::format(
                "Particle density: {:.0f}%",
                m_particleDensityMultiplier * 100.0f
            )
        );
    }

    void D3D12Context::
        AdjustParticleMotionSpeed(
            float delta) noexcept
    {
        m_particleMotionSpeedMultiplier =
            std::clamp(
                m_particleMotionSpeedMultiplier + delta,
                0.10f,
                3.0f
            );

        core::LogInfo(
            std::format(
                "Particle motion speed: {:.2f}x",
                m_particleMotionSpeedMultiplier
            )
        );
    }

    void D3D12Context::
        AdjustColorGradient(
            float delta) noexcept
    {
        m_galaxyParameters
            .colorGradientExponent =
            std::clamp(
                m_galaxyParameters
                    .colorGradientExponent +
                delta,
                0.20f,
                3.0f
            );

        core::LogInfo(
            std::format(
                "Galaxy color gradient exponent: {:.2f}",
                m_galaxyParameters
                    .colorGradientExponent
            )
            );
    }

    void D3D12Context::
        RandomizeGalaxyColors()
    {
        static bool useWackyColorPair = false;

        useWackyColorPair = !useWackyColorPair;

        std::uniform_int_distribution<std::size_t> warmIndex(
            0,
            GalaxyWarmColorPresets.size() - 1u
        );

        std::uniform_int_distribution<std::size_t> coolIndex(
            0,
            GalaxyCoolColorPresets.size() - 1u
        );

        std::uniform_int_distribution<std::size_t> wackyIndex(
            0,
            GalaxyWackyColorPresets.size() - 1u
        );

        auto& rng =
            GalaxyColorRng();

        if (useWackyColorPair)
        {
            m_galaxyParameters
                .coreColor =
                GalaxyWackyColorPresets[wackyIndex(rng)];

            m_galaxyParameters
                .outerArmColor =
                GalaxyCoolColorPresets[coolIndex(rng)];
        }
        else
        {
            m_galaxyParameters
                .coreColor =
                GalaxyWarmColorPresets[warmIndex(rng)];

            m_galaxyParameters
                .outerArmColor =
                GalaxyWackyColorPresets[wackyIndex(rng)];
        }

        core::LogInfo(
            std::format(
                "Galaxy colors randomized. core: [{:.2f}, {:.2f}, {:.2f}] outer: [{:.2f}, {:.2f}, {:.2f}] {}",
                m_galaxyParameters.coreColor[0],
                m_galaxyParameters.coreColor[1],
                m_galaxyParameters.coreColor[2],
                m_galaxyParameters.outerArmColor[0],
                m_galaxyParameters.outerArmColor[1],
                m_galaxyParameters.outerArmColor[2],
                useWackyColorPair
                    ? "core is wacky"
                    : "outer arm is wacky"
            )
        );
    }

    void D3D12Context::
        CycleParticleDebugDrawCount()
    {
        if (m_particleSystem == nullptr)
        {
            return;
        }

        const std::uint32_t fullCount =
            m_particleSystem->Count();

        const std::uint32_t lowCount =
            std::max(
                256u,
                fullCount / 16u
            );

        const std::uint32_t mediumCount =
            std::max(
                lowCount,
                fullCount / 4u
            );


        if (
            m_particleDebugDrawCount >=
            fullCount)
        {
            m_particleDebugDrawCount =
                lowCount;
        }
        else if (
            m_particleDebugDrawCount <
            mediumCount)
        {
            m_particleDebugDrawCount =
                mediumCount;
        }
        else
        {
            m_particleDebugDrawCount =
                fullCount;
        }

        m_particleDensityMultiplier =
            std::clamp(
                static_cast<float>(m_particleDebugDrawCount) /
                static_cast<float>(fullCount),
                0.05f,
                1.0f
            );


        core::LogInfo(
            std::format(
                "Particle density cycle: {} / {}",
                m_particleDebugDrawCount,
                fullCount
            )
        );
    }

    void D3D12Context::
        CycleInteractiveGravitySourceCount()
    {
        ++m_interactiveGravitySourceCount;

        if (
            m_interactiveGravitySourceCount >
            MaxInteractiveGravitySources)
        {
            m_interactiveGravitySourceCount =
                1;
        }


        //
        // Don't leave the selected index pointing
        // at an inactive source.
        //
        if (
            m_selectedInteractiveGravitySource >=
            m_interactiveGravitySourceCount)
        {
            m_selectedInteractiveGravitySource =
                0;
        }


        core::LogInfo(
            std::format(
                "Interactive gravity source count: {}",
                m_interactiveGravitySourceCount
            )
        );
    }

    void D3D12Context::
        CycleSelectedInteractiveGravitySource()
    {
        m_selectedInteractiveGravitySource =
            (
                m_selectedInteractiveGravitySource
                + 1
                )
            %
            m_interactiveGravitySourceCount;


        core::LogInfo(
            std::format(
                "Selected interactive gravity source: {}",
                m_selectedInteractiveGravitySource
                + 1
            )
        );
    }

    void D3D12Context::
        TriggerParticleExplosion()
    {
        //
        // If we explode while reforming,
        // cancel the current reform timer.
        //
        m_particleReformTimeRemaining =
            0.0f;

        m_particleSimulationMode =
            ParticleSimulationMode::Explosion;

        core::LogInfo(
            "Particle explosion queued"
        );
    }

    void D3D12Context::
        TriggerParticleReformation()
    {
        m_particleSimulationMode =
            ParticleSimulationMode::Reform;

        m_particleReformTimeRemaining =
            ParticleReformDurationSeconds;

        core::LogInfo(
            "Particle reformation started"
        );
    }

    void D3D12Context::CycleDebrisDrawCount()
    {
        if (m_debrisField == nullptr)
        {
            return;
        }

        const std::uint32_t fullCount =
            m_debrisField->Count();

        const std::uint32_t lowCount =
            std::max(
                1u,
                fullCount / 4u
            );

        const std::uint32_t mediumCount =
            std::max(
                lowCount,
                fullCount / 2u
            );

        if (m_debrisDrawCount >= fullCount)
        {
            m_debrisDrawCount =
                lowCount;
        }
        else if (
            m_debrisDrawCount <
            mediumCount)
        {
            m_debrisDrawCount =
                mediumCount;
        }
        else
        {
            m_debrisDrawCount =
                fullCount;
        }

        core::LogInfo(
            std::format(
                "Debris draw count: {}",
                m_debrisDrawCount
            )
        );
    }

    void D3D12Context::ToggleDebrisAnimation()
    {
        m_debrisAnimationEnabled =
            !m_debrisAnimationEnabled;

        core::LogInfo(
            m_debrisAnimationEnabled
            ? "Debris animation resumed"
            : "Debris animation frozen"
        );
    }

    void D3D12Context::ToggleDebrisCulling()
    {
        m_debrisCullingEnabled =
            !m_debrisCullingEnabled;

        if (!m_debrisCullingEnabled)
        {
            m_debrisCullingDebugEnabled = false;
        }

        core::LogInfo(
            m_debrisCullingEnabled
            ? "Debris GPU culling enabled"
            : "Debris GPU culling disabled"
        );
    }

    void D3D12Context::ToggleDebrisCullingDebug()
    {
        m_debrisCullingDebugEnabled =
            !m_debrisCullingDebugEnabled;

        if (m_debrisCullingDebugEnabled)
        {
            m_debrisCullingEnabled = true;
        }

        core::LogInfo(
            m_debrisCullingDebugEnabled
            ? "Debris culling debug enabled"
            : "Debris culling debug disabled"
        );
    }

    void D3D12Context::ToggleDebrisIndirect()
    {
        m_debrisIndirectEnabled =
            !m_debrisIndirectEnabled;

        core::LogInfo(
            m_debrisIndirectEnabled
            ? "Debris indirect drawing enabled"
            : "Debris indirect drawing disabled"
        );
    }

    FrameDiagnostics D3D12Context::GetFrameDiagnostics() const noexcept
    {
        const UINT64 completedFenceValue = m_fence != nullptr ? m_fence->GetCompletedValue() : 0;

        return FrameDiagnostics{
            m_frameResourceIndex,
            m_backBufferIndex,
            completedFenceValue,
            m_lastSubmittedFenceValue,
            m_submittedFrameCount
        };
    }

    void D3D12Context::FlushGpu()
    {
        if (m_commandQueue == nullptr ||
            m_fence == nullptr ||
            m_fenceEvent == nullptr)
        {
            return;
        }

        const UINT64 flushFenceValue =
            m_nextFenceValue;

        ++m_nextFenceValue;

        core::ThrowIfFailed(
            m_commandQueue->Signal(
                m_fence.Get(),
                flushFenceValue
            ),
            "Failed to signal shutdown fence"
        );

        if (m_fence->GetCompletedValue() <
            flushFenceValue)
        {
            core::ThrowIfFailed(
                m_fence->SetEventOnCompletion(
                    flushFenceValue,
                    m_fenceEvent
                ),
                "Failed to configure shutdown fence event"
            );

            const DWORD waitResult =
                WaitForSingleObject(
                    m_fenceEvent,
                    INFINITE
                );

            if (waitResult == WAIT_FAILED)
            {
                core::ThrowLastWin32Error(
                    "Shutdown GPU wait failed"
                );
            }

            if (waitResult != WAIT_OBJECT_0)
            {
                throw std::runtime_error(
                    "Unexpected shutdown GPU wait result"
                );
            }
        }
    }
}
