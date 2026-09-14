#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include "graphics/frame/FrameResource.h"
#include "graphics/material/Material.h"
#include "../../scene/ReactorScene.h"
#include "../../simulation/GalaxyParameters.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "../../simulation/ParticleSystem.h"
#include <DirectXMath.h>

namespace gf::graphics::pipeline
{
    class BasicPipeline;
    class PresentationPipeline;
    class ParticleSimulationPipeline;
    class ParticleDebugPipeline;
    class ParticleBillboardPipeline;
    class DebrisCullingPipeline;
    class BloomExtractPipeline;
    class BloomBlurPipeline;
}

namespace gf::scene
{
    class Camera;
    class DebrisField;
}

namespace gf::graphics::mesh
{
    class Mesh;
}

namespace gf::graphics::texture
{
    class Texture2D;
}

namespace gf::graphics::d3d12
{
    struct FrameDiagnostics
    {
        std::uint32_t nextFrameResourceIndex = 0;
        std::uint32_t nextBackBufferIndex = 0;

        std::uint64_t completedFenceValue = 0;
        std::uint64_t lastSubmittedFenceValue = 0;

        std::uint64_t submittedFrameCount = 0;
    };

    inline constexpr std::uint32_t MaxInteractiveGravitySources = 3;

    class D3D12Context final
    {
    public:
        D3D12Context(
            HWND windowHandle,
            std::uint32_t width,
            std::uint32_t height
        );

        ~D3D12Context() noexcept;
        void Render(float elapsedSeconds, float deltaSeconds, const gf::scene::Camera& camera);
        void Resize(std::uint32_t width,std::uint32_t height);
        void ToggleDepthTesting();
        void AdjustCoreParticleIntensity(
            float delta
        ) noexcept;

        [[nodiscard]]
        bool IsDepthTestingEnabled()
            const noexcept
        {
            return m_depthTestingEnabled;
        }

        [[nodiscard]]
        float CoreParticleIntensity()
            const noexcept
        {
            return
                m_galaxyParameters
                .coreParticleIntensity;
        }

        void AdjustExposure(
            float delta
        ) noexcept;

        void ToggleBloom();
        void IncreaseBloomIntensity();
        void DecreaseBloomIntensity();
        void IncreaseBloomThreshold();
        void DecreaseBloomThreshold();
        void IncreaseExposure();
        void DecreaseExposure();

        void ToggleParticleSimulation();
        void ToggleParticleRendering();
        void ToggleParticleDepthTesting();

        void ToggleInteractiveGravitySource();

        void MoveInteractiveGravitySource(
            float deltaX,
            float deltaY,
            float deltaZ
        ) noexcept;

        void AdjustGalaxySpin(
            float delta
        ) noexcept;

        void AdjustDifferentialRotationStrength(
            float delta
        ) noexcept;

        void AdjustSpiralStrength(
            float delta
        ) noexcept;

        void AdjustArmConcentration(
            float delta
        ) noexcept;

        void AdjustParticleDensity(
            float delta
        ) noexcept;

        void AdjustParticleMotionSpeed(
            float delta
        ) noexcept;

        void AdjustColorGradient(
            float delta
        ) noexcept;

        void RandomizeGalaxyColors();

        void CycleParticleDebugDrawCount();

        void CycleInteractiveGravitySourceCount();

        void CycleSelectedInteractiveGravitySource();

        void TriggerParticleExplosion();

        void TriggerParticleReformation();

        void CycleDebrisDrawCount();
        void ToggleDebrisAnimation();
        void ToggleDebrisCulling();
        void ToggleDebrisCullingDebug();
        void ToggleDebrisIndirect();

        [[nodiscard]]
        bool IsDebrisIndirectEnabled()
            const noexcept
        {
            return m_debrisIndirectEnabled;
        }

        [[nodiscard]]
        std::uint32_t
            DebrisDrawCount() const noexcept
        {
            return m_debrisDrawCount;
        }

        [[nodiscard]]
        bool
            IsDebrisAnimationEnabled()
            const noexcept
        {
            return m_debrisAnimationEnabled;
        }

        [[nodiscard]]
        bool IsDebrisCullingEnabled()
            const noexcept
        {
            return m_debrisCullingEnabled;
        }

        [[nodiscard]]
        bool IsDebrisCullingDebugEnabled()
            const noexcept
        {
            return m_debrisCullingDebugEnabled;
        }

        [[nodiscard]]
        bool IsInteractiveGravitySourceEnabled()
            const noexcept
        {
            return m_interactiveGravityEnabled;
        }

        [[nodiscard]]
        DirectX::XMFLOAT3
            SelectedInteractiveGravitySourcePosition()
            const noexcept
        {
            return
                m_interactiveGravitySources[
                    m_selectedInteractiveGravitySource
                ].position;
        }

        [[nodiscard]]
        std::uint32_t
            InteractiveGravitySourceCount()
            const noexcept
        {
            return
                m_interactiveGravitySourceCount;
        }


        [[nodiscard]]
        std::uint32_t
            SelectedInteractiveGravitySourceIndex()
            const noexcept
        {
            return
                m_selectedInteractiveGravitySource;
        }

        [[nodiscard]]
        bool IsParticleRenderingEnabled()
            const noexcept
        {
            return m_particleRenderingEnabled;
        }


        [[nodiscard]]
        bool IsParticleDepthTestingEnabled()
            const noexcept
        {
            return m_particleDepthTestingEnabled;
        }

        [[nodiscard]]
        bool IsParticleSimulationEnabled()
            const noexcept
        {
            return m_particleSimulationEnabled;
        }

        [[nodiscard]]
        float GalaxySpin()
            const noexcept
        {
            return m_galaxyParameters.spin;
        }

        [[nodiscard]]
        float DifferentialRotationStrength()
            const noexcept
        {
            return
                m_galaxyParameters
                .differentialRotationStrength;
        }

        [[nodiscard]]
        float RadialPerturbationStrength()
            const noexcept
        {
            return
                m_galaxyParameters
                .radialPerturbationStrength;
        }

        [[nodiscard]]
        float VerticalOscillationFrequency()
            const noexcept
        {
            return
                m_galaxyParameters
                .verticalOscillationFrequency;
        }

        [[nodiscard]]
        float SpiralSpread()
            const noexcept
        {
            return m_galaxyParameters.spiralSpread;
        }

        [[nodiscard]]
        float ArmConcentration()
            const noexcept
        {
            return m_galaxyParameters.armConcentration;
        }

        [[nodiscard]]
        float ColorGradientExponent()
            const noexcept
        {
            return m_galaxyParameters.colorGradientExponent;
        }

        [[nodiscard]]
        float ParticleDensity()
            const noexcept
        {
            if (m_particleSystem == nullptr)
            {
                return 0.0f;
            }

            return std::clamp(
                static_cast<float>(m_particleDebugDrawCount) /
                static_cast<float>(m_particleSystem->Count()),
                0.0f,
                1.0f
            );
        }

        [[nodiscard]]
        float ParticleMotionSpeed()
            const noexcept
        {
            return m_particleMotionSpeedMultiplier;
        }

        [[nodiscard]]
        std::uint32_t ParticleDebugDrawCount()
            const noexcept
        {
            return m_particleDebugDrawCount;
        }

        [[nodiscard]]
        float Exposure() const noexcept
        {
            return m_exposure;
        }

        [[nodiscard]]
        bool IsBloomEnabled() const noexcept
        {
            return m_bloomEnabled;
        }

        [[nodiscard]]
        float BloomThreshold() const noexcept
        {
            return m_bloomThreshold;
        }

        [[nodiscard]]
        float BloomIntensity() const noexcept
        {
            return m_bloomIntensity;
        }

        [[nodiscard]] FrameDiagnostics GetFrameDiagnostics()
            const noexcept;

        D3D12Context(const D3D12Context&) = delete;
        D3D12Context& operator=(const D3D12Context&) = delete;

        D3D12Context(D3D12Context&&) = delete;
        D3D12Context& operator=(D3D12Context&&) = delete;
    private:

        enum class ParticleSimulationMode :
            std::uint32_t
        {
            Orbit = 0,
            Explosion = 1,
            Reform = 2
        };

        struct InteractiveGravitySource
        {
            DirectX::XMFLOAT3 position{};
            float strength = 0.0f;
        };

        struct SrvAllocation
        {
            D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
            D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
            UINT index = 0;
        };

        struct ParticleBufferViews
        {
            SrvAllocation srv{};
            SrvAllocation uav{};
        };

        void CreateSrvHeap();

        [[nodiscard]]
        SrvAllocation AllocateSrv();

        D3D12_GPU_DESCRIPTOR_HANDLE CreateTextureSrv(
            const gf::graphics::texture::Texture2D& texture
        );

        void CreateTestTextureSrvs();

        static constexpr UINT SrvHeapCapacity = 32;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
            m_srvHeap;

        UINT m_srvDescriptorSize = 0;
        UINT m_nextSrvDescriptor = 0;

        gf::graphics::material::Material m_coreMaterial;

        gf::graphics::material::Material m_loadedMaterial;

        [[nodiscard]]
        const gf::graphics::mesh::Mesh&
            MeshFor(
                gf::scene::ReactorMeshKind kind
            ) const;

        [[nodiscard]]
        const gf::graphics::material::Material&
            MaterialFor(
                gf::scene::ReactorMaterialKind kind
            ) const;

        void RecordParticleSimulation(
            float deltaSeconds
        );

        void RecordBloomExtraction();

        void RecordBloomBlur();

        void RecordParticleDebugDraw(
            gf::graphics::frame::FrameResource&
            frameResource,
            const gf::scene::Camera& camera,
            std::uint32_t constantSlot
        );

        void RecordParticleBillboardDraw(
            gf::graphics::frame::FrameResource&
            frameResource,
            const gf::scene::Camera& camera,
            std::uint32_t constantSlot,
            float elapsedSeconds
        );

        gf::simulation::GalaxyParameters m_galaxyParameters =
            gf::simulation::DefaultGalaxyParameters;

        void RecordDebrisCulling(
            const gf::scene::Camera& camera
        );

        // Debris Stuff
        std::unique_ptr<gf::scene::DebrisField> m_debrisField;
        SrvAllocation m_debrisInstancesSrv{};
        void DrawDebris(
            gf::graphics::frame::FrameResource&
            frameResource,
            const gf::scene::Camera& camera,
            float elapsedSeconds,
            std::uint32_t constantSlot
        );
        std::uint32_t m_debrisDrawCount = 0;
        bool m_debrisAnimationEnabled = true;
        bool m_debrisCullingEnabled = true;
        bool m_debrisCullingDebugEnabled = false;
        bool m_debrisIndirectEnabled = false;
        float m_debrisAnimationTime = 0.0f;

        static constexpr std::uint32_t SwapChainBufferCount = 3;
        static constexpr std::uint32_t FrameResourceCount = 2;        
        static constexpr UINT HdrSceneRtvIndex = SwapChainBufferCount;
        static constexpr UINT BloomExtractRtvIndex = HdrSceneRtvIndex + 1;
        static constexpr UINT BloomBlurTempRtvIndex = BloomExtractRtvIndex + 1;
        static constexpr UINT RtvHeapCapacity = SwapChainBufferCount + 3;

        void EnableDebugLayer();
        void CreateFactory();
        void SelectAdapter();
        void CreateDevice();
        void CreateCommandQueue();
        void CreateSwapChain();
        void CreateRtvHeap();
        void CreateBackBuffers();
        void CreateCommandObjects();
        void CreateSynchronizationObjects();
        void CreateReactorMeshes();
        void CreateTestTexture();
        void CreatePipeline();
        void CreateDsvHeap();
        void CreateDepthBuffer();
        void CreateHdrSceneTarget();
        void CreateBloomExtractTarget();
        void CreateBloomBlurTempTarget();
        void CreateParticleSystem();
        void CreateParticleBufferViews();
        void CreateDebrisField();
        void CreateDebrisInstanceView();
        void CreateDebrisCullingResources();
        void CreateDebrisIndirectResources();
        void DrawMesh(
            const gf::graphics::mesh::Mesh& mesh,
            gf::graphics::frame::FrameResource&
            frameResource,
            const gf::scene::Camera& camera,
            const DirectX::XMMATRIX& model,
            const gf::graphics::material::Material&
            material,
            float elapsedSeconds,
            std::uint32_t objectIndex
        );
        void UpdateViewportAndScissor();
        gf::graphics::frame::FrameResource& BeginFrame();
        void EndFrame(
            gf::graphics::frame::FrameResource&
            frameResource
        );
        void WaitForFrame(
            const gf::graphics::frame::FrameResource&
            frameResource
        );
        void RecordFrame(float elapsedSeconds, float deltaSeconds, gf::graphics::frame::FrameResource&frameResource,const gf::scene::Camera& camera);
        void FlushGpu();

        HWND m_windowHandle = nullptr;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;

        UINT m_factoryFlags = 0;

        Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,SwapChainBufferCount> m_backBuffers;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_hdrSceneColor;
        D3D12_CPU_DESCRIPTOR_HANDLE m_hdrSceneRtvHandle{};
        SrvAllocation m_hdrSceneSrv{};
        Microsoft::WRL::ComPtr<ID3D12Resource> m_bloomExtractTexture;
        D3D12_CPU_DESCRIPTOR_HANDLE m_bloomExtractRtvHandle{};
        SrvAllocation m_bloomExtractSrv{};
        Microsoft::WRL::ComPtr<ID3D12Resource> m_bloomBlurTempTexture;
        D3D12_CPU_DESCRIPTOR_HANDLE m_bloomBlurTempRtvHandle{};
        SrvAllocation m_bloomBlurTempSrv{};
        D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,SwapChainBufferCount> m_rtvHandles{};
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        std::unique_ptr<gf::graphics::pipeline::BasicPipeline> m_basicPipeline;
        std::unique_ptr<gf::graphics::pipeline::PresentationPipeline> m_presentationPipeline;
        std::unique_ptr<gf::graphics::pipeline::ParticleSimulationPipeline> m_particleSimulationPipeline;
        std::unique_ptr<gf::graphics::pipeline::ParticleDebugPipeline> m_particleDebugPipeline;
        std::unique_ptr<gf::graphics::pipeline::ParticleBillboardPipeline> m_particleBillboardPipeline;
        std::unique_ptr<gf::graphics::pipeline::DebrisCullingPipeline> m_debrisCullingPipeline;
        std::unique_ptr<gf::graphics::pipeline::BloomExtractPipeline> m_bloomExtractPipeline;
        std::unique_ptr<gf::graphics::pipeline::BloomBlurPipeline> m_bloomBlurPipeline;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_debrisVisibilityFlags;
        SrvAllocation m_debrisVisibilityFlagsSrv{};
        SrvAllocation m_debrisVisibilityFlagsUav{};
        std::unique_ptr<gf::simulation::ParticleSystem> m_particleSystem;
        std::array<ParticleBufferViews,2> m_particleBufferViews{};
        Microsoft::WRL::ComPtr<ID3D12Resource> m_debrisVisibleInstanceIndices;
        SrvAllocation m_debrisVisibleInstanceIndicesSrv{};
        SrvAllocation m_debrisVisibleInstanceIndicesUav{};
        Microsoft::WRL::ComPtr<ID3D12Resource> m_debrisVisibleCount;
        SrvAllocation m_debrisVisibleCountSrv{};
        SrvAllocation m_debrisVisibleCountUav{};
        Microsoft::WRL::ComPtr<ID3D12Resource> m_debrisIndirectArguments;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_debrisDrawCommandSignature;
        // ClearUnorderedAccessViewUint needs a matching
        // CPU-only UAV descriptor.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_debrisVisibleCountClearHeap;
        std::unique_ptr<gf::graphics::mesh::Mesh> m_ringMesh;
        std::unique_ptr<gf::graphics::mesh::Mesh> m_coreMesh;
        std::unique_ptr<gf::graphics::mesh::Mesh> m_braceMesh;
        gf::scene::ReactorScene m_reactorScene;
        std::unique_ptr<gf::graphics::texture::Texture2D> m_testTexture;
        std::unique_ptr<gf::graphics::texture::Texture2D> m_materialTexture;
        D3D12_VIEWPORT m_viewport{};
        D3D12_RECT m_scissorRect{};
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        std::vector<gf::graphics::frame::FrameResource> m_frameResources;
        UINT64 m_nextFenceValue = 1;
        UINT64 m_lastSubmittedFenceValue = 0;
        UINT64 m_submittedFrameCount = 0;
        static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;
        static constexpr DXGI_FORMAT HdrSceneFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        HANDLE m_fenceEvent = nullptr;
        bool m_depthTestingEnabled = true;
        float m_exposure = 1.0f;
        bool m_particleSimulationEnabled = true;
        float m_particleExplosionStrength = 2.75f;
        float m_particleExplosionTangentialStrength = 0.65f;
        float m_particleReformPositionStrength = 7.0f;
        float m_particleReformVelocityStrength = 5.5f;
        float m_particleReformMaxAcceleration = 30.0f;
        float m_particleReformTimeRemaining = 0.0f;
        static constexpr float ParticleReformDurationSeconds = 4.0f;
        std::uint32_t m_particleDebugDrawCount = 0;
        float m_particleDensityMultiplier = 1.0f;
        float m_particleMotionSpeedMultiplier = 1.0f;
        bool m_particleRenderingEnabled = true;
        bool m_particleDepthTestingEnabled = true;
        ParticleSimulationMode m_particleSimulationMode = ParticleSimulationMode::Orbit;
        bool m_interactiveGravityEnabled = false;
        std::array<
            InteractiveGravitySource,
            MaxInteractiveGravitySources
        > m_interactiveGravitySources{
            InteractiveGravitySource{
                { 2.75f, 0.0f, 0.0f },
                0.80f
            },

            InteractiveGravitySource{
                { -2.75f, 0.0f, 0.0f },
                0.55f
            },

            InteractiveGravitySource{
                { 0.0f, 2.25f, 1.50f },
                0.50f
            }
        };
        std::uint32_t m_interactiveGravitySourceCount = 1;
        std::uint32_t m_selectedInteractiveGravitySource = 0;
        float m_bloomThreshold = 1.0f;
        float m_bloomIntensity = 0.35f;
        bool m_bloomEnabled = true;
        UINT m_rtvDescriptorSize = 0;
        UINT m_backBufferIndex = 0;
        UINT m_frameResourceIndex = 0;
    };
}
