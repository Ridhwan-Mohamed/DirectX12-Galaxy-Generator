#include "Application.h"

#include "../core/Log.h"

#include <chrono>
#include <thread>
#include <format>
#include <algorithm>
#include <array>
#include <string_view>
#include <string>

using namespace std::chrono_literals;

namespace gf::app
{
    Application::Application()
        : m_window(
            L"Gravity Forge",
            1280,
            720
        ),
        m_camera(
            m_window.ClientWidth(),
            m_window.ClientHeight()
        ),
        m_graphics(
            m_window.NativeHandle(),
            m_window.ClientWidth(),
            m_window.ClientHeight()
        )
    {
    }

    int Application::Run(int showCommand)
    {
        core::LogInfo("Application started");

        m_window.Show(showCommand);

        const auto startTime =
            std::chrono::steady_clock::now();

        auto previousFrameTime = startTime;

        bool depthWasDown = false;
        bool simulationPauseWasDown = false;
        bool interactiveGravityToggleWasDown = false;
        bool gravitySourceCountWasDown = false;
        bool gravitySourceSelectionWasDown = false;
        bool differentialDownWasDown = false;
        bool differentialUpWasDown = false;
        bool particleCountWasDown = false;
        bool particleRenderingWasDown = false;
        bool particleDepthWasDown = false;
        bool explosionWasDown = false;
        bool reformWasDown = false;
        bool bloomToggleWasDown = false;
        bool bloomIntensityIncreaseWasDown = false;
        bool bloomIntensityDecreaseWasDown = false;
        bool bloomThresholdIncreaseWasDown = false;
        bool bloomThresholdDecreaseWasDown = false;
        bool bloomExposureIncreaseWasDown = false;
        bool bloomExposureDecreaseWasDown = false;
        bool cameraPresetWasDown = false;
        bool galaxyColorsRandomizeWasDown = false;

        struct CameraPreset
        {
            DirectX::XMFLOAT3 position;
            float yaw;
            float pitch;
            float roll;
            std::string_view label;
        };

        const std::array<CameraPreset, 4> cameraPresets{
                CameraPreset{
                DirectX::XMFLOAT3{0.0f, 0.4f, -4.4f},
                0.0f,
                -0.05f,
                0.0f,
                "wide front",
            },
            CameraPreset{
                DirectX::XMFLOAT3{0.0f, 2.8f, -2.4f},
                0.0f,
                -1.05f,
                0.0f,
                "top oblique",
            },
            CameraPreset{
                DirectX::XMFLOAT3{2.6f, 0.2f, 2.0f},
                2.6f,
                0.10f,
                0.0f,
                "edge",
            },
            CameraPreset{
                DirectX::XMFLOAT3{0.0f, 0.3f, 0.75f},
                3.2f,
                0.24f,
                0.0f,
                "core close",
            }
        };

        int cameraPresetIndex = 0;

        auto nextStatusUpdate = startTime;

        while (m_window.ProcessMessages())
        {
            const auto currentTime =
                std::chrono::steady_clock::now();

            const float elapsedSeconds =
                std::chrono::duration<float>(
                    currentTime - startTime
                ).count();

            const float deltaSeconds =
                std::chrono::duration<float>(
                    currentTime -
                    previousFrameTime
                ).count();

            previousFrameTime = currentTime;

            const float controlDeltaSeconds =
                std::clamp(
                    deltaSeconds,
                    0.0f,
                    0.1f
                );

            constexpr float
                interactiveGravityMoveSpeed =
                1.50f;

            //
            // Interactive gravity source movement.
            //
            // J / L = world X - / +
            // U / O = world Y + / -
            // K / I = world Z - / +
            //

            const float gravitySourceMoveX =
                static_cast<float>(
                    m_window.IsKeyDown('L')
                    )
                -
                static_cast<float>(
                    m_window.IsKeyDown('J')
                    );


            const float gravitySourceMoveY =
                static_cast<float>(
                    m_window.IsKeyDown('U')
                    )
                -
                static_cast<float>(
                    m_window.IsKeyDown('O')
                    );


            const float gravitySourceMoveZ =
                static_cast<float>(
                    m_window.IsKeyDown('I')
                    )
                -
                static_cast<float>(
                    m_window.IsKeyDown('K')
                    );


            m_graphics
                .MoveInteractiveGravitySource(
                    gravitySourceMoveX *
                    interactiveGravityMoveSpeed *
                    controlDeltaSeconds,

                    gravitySourceMoveY *
                    interactiveGravityMoveSpeed *
                    controlDeltaSeconds,

                    gravitySourceMoveZ *
                    interactiveGravityMoveSpeed *
                    controlDeltaSeconds
                );

            if (!m_window.IsMinimized())
            {
                gf::scene::CameraInput cameraInput{};

                cameraInput.moveForward =
                    m_window.IsKeyDown('W');

                cameraInput.moveBackward =
                    m_window.IsKeyDown('S');

                cameraInput.moveLeft =
                    m_window.IsKeyDown('A');

                cameraInput.moveRight =
                    m_window.IsKeyDown('D');

                cameraInput.moveDown =
                    m_window.IsKeyDown('Q');

                cameraInput.moveUp =
                    m_window.IsKeyDown('E');

                cameraInput.turnLeft =
                    m_window.IsKeyDown(VK_LEFT);

                cameraInput.turnRight =
                    m_window.IsKeyDown(VK_RIGHT);

                cameraInput.lookUp =
                    m_window.IsKeyDown(VK_UP);

                cameraInput.lookDown =
                    m_window.IsKeyDown(VK_DOWN);

                cameraInput.rollLeft =
                    m_window.IsKeyDown('Z');

                cameraInput.rollRight =
                    m_window.IsKeyDown('C');

                m_camera.SetAspectRatio(
                    m_window.ClientWidth(),
                    m_window.ClientHeight()
                );

                m_camera.Update(
                    cameraInput,
                    deltaSeconds
                );
                //
                // 1 = toggle depth testing
                //
                const bool depthIsDown =
                    m_window.IsKeyDown('1');

                if (
                    depthIsDown &&
                    !depthWasDown)
                {
                    m_graphics.ToggleDepthTesting();
                }

                depthWasDown = depthIsDown;


                //
                // 8 = pause / resume particle simulation
                //
                const bool simulationPauseIsDown =
                    m_window.IsKeyDown('8');

                if (
                    simulationPauseIsDown &&
                    !simulationPauseWasDown)
                {
                    m_graphics
                        .ToggleParticleSimulation();
                }

                simulationPauseWasDown =
                    simulationPauseIsDown;


                //
                // P = cycle particle debug draw count
                //
                const bool particleCountIsDown =
                    m_window.IsKeyDown('P');

                if (
                    particleCountIsDown &&
                    !particleCountWasDown)
                {
                    m_graphics
                        .CycleParticleDebugDrawCount();
                }

                particleCountWasDown =
                    particleCountIsDown;

                //
                // B = particle billboard rendering on / off.
                //
                const bool particleRenderingIsDown =
                    m_window.IsKeyDown('B');

                if (
                    particleRenderingIsDown &&
                    !particleRenderingWasDown)
                {
                    m_graphics
                        .ToggleParticleRendering();
                }

                particleRenderingWasDown =
                    particleRenderingIsDown;

                //
                // G = enable / disable the interactive
                // gravity source.
                //
                const bool interactiveGravityToggleIsDown =
                    m_window.IsKeyDown('G');

                if (
                    interactiveGravityToggleIsDown &&
                    !interactiveGravityToggleWasDown)
                {
                    m_graphics
                        .ToggleInteractiveGravitySource();
                }

                interactiveGravityToggleWasDown =
                    interactiveGravityToggleIsDown;

                //
                // T = cycle how many interactive
                // gravity sources are active.
                //
                const bool gravitySourceCountIsDown =
                    m_window.IsKeyDown('T');

                if (
                    gravitySourceCountIsDown &&
                    !gravitySourceCountWasDown)
                {
                    m_graphics
                        .CycleInteractiveGravitySourceCount();
                }

                gravitySourceCountWasDown =
                    gravitySourceCountIsDown;

                //
                // H = select which active gravity
                // source the movement keys control.
                //
                const bool gravitySourceSelectionIsDown =
                    m_window.IsKeyDown('H');

                if (
                    gravitySourceSelectionIsDown &&
                    !gravitySourceSelectionWasDown)
                {
                    m_graphics
                        .CycleSelectedInteractiveGravitySource();
                }

                gravitySourceSelectionWasDown =
                    gravitySourceSelectionIsDown;   

                //
                // V = particle depth-test debug toggle.
                //
                // Normal:
                //      depth test ON
                //      depth writes OFF
                //
                // Debug:
                //      depth test OFF
                //      depth writes OFF
                //
                const bool particleDepthIsDown =
                    m_window.IsKeyDown('V');

                if (
                    particleDepthIsDown &&
                    !particleDepthWasDown)
                {
                    m_graphics
                        .ToggleParticleDepthTesting();
                }

                particleDepthWasDown =
                    particleDepthIsDown;

                //
                // E = trigger a one-shot GPU
                // particle explosion.
                //
                const bool explosionIsDown =
                    m_window.IsKeyDown('F');

                if (
                    explosionIsDown &&
                    !explosionWasDown)
                {
                    m_graphics
                        .TriggerParticleExplosion();
                }

                explosionWasDown =
                    explosionIsDown;

                //
                // R = reform the current GPU particle
                // field back toward its orbital manifold.
                //
                const bool reformIsDown =
                    m_window.IsKeyDown('R');


                if (
                    reformIsDown &&
                    !reformWasDown)
                {
                    m_graphics
                        .TriggerParticleReformation();
                }


                reformWasDown =
                    reformIsDown;

                //
                // / = randomise galaxy warm and cool colors.
                //
                const bool galaxyColorsRandomizeIsDown =
                    m_window.IsKeyDown(VK_OEM_2) ||
                    m_window.IsKeyDown(VK_DIVIDE);

                if (
                    galaxyColorsRandomizeIsDown &&
                    !galaxyColorsRandomizeWasDown)
                {
                    m_graphics
                        .RandomizeGalaxyColors();
                }

                galaxyColorsRandomizeWasDown =
                    galaxyColorsRandomizeIsDown;

                //
                // F1 = cycle camera preset.
                //
                const bool cameraPresetIsDown =
                    m_window.IsKeyDown(VK_F1);

                if (
                    cameraPresetIsDown &&
                    !cameraPresetWasDown)
                {
                    cameraPresetIndex =
                        (cameraPresetIndex + 1) %
                        static_cast<int>(
                            cameraPresets.size()
                        );

                const auto& preset =
                    cameraPresets[
                        static_cast<std::size_t>(
                            cameraPresetIndex
                        )
                    ];

                    m_camera.SetPose(
                        preset.position,
                        preset.yaw,
                        preset.pitch,
                        preset.roll
                    );

                    core::LogInfo(
                        std::format(
                            "Camera preset {}: {}",
                            cameraPresetIndex,
                            preset.label
                        )
                    );
                }

                cameraPresetWasDown = cameraPresetIsDown;

                //
                // Y = bloom toggle.
                //
                const bool bloomToggleIsDown =
                    m_window.IsKeyDown('Y');

                if (
                    bloomToggleIsDown &&
                    !bloomToggleWasDown)
                {
                    m_graphics
                        .ToggleBloom();
                }

                bloomToggleWasDown =
                    bloomToggleIsDown;


                //
                // [ / ] = bloom intensity down / up.
                //
                const bool bloomIntensityDecreaseIsDown =
                    m_window.IsKeyDown(VK_OEM_4);

                if (
                    bloomIntensityDecreaseIsDown &&
                    !bloomIntensityDecreaseWasDown)
                {
                    m_graphics
                        .DecreaseBloomIntensity();
                }

                bloomIntensityDecreaseWasDown =
                    bloomIntensityDecreaseIsDown;


                const bool bloomIntensityIncreaseIsDown =
                    m_window.IsKeyDown(VK_OEM_6);

                if (
                    bloomIntensityIncreaseIsDown &&
                    !bloomIntensityIncreaseWasDown)
                {
                    m_graphics
                        .IncreaseBloomIntensity();
                }

                bloomIntensityIncreaseWasDown =
                    bloomIntensityIncreaseIsDown;


                //
                // ; / ' = bloom threshold down / up.
                //
                const bool bloomThresholdDecreaseIsDown =
                    m_window.IsKeyDown(VK_OEM_1);

                if (
                    bloomThresholdDecreaseIsDown &&
                    !bloomThresholdDecreaseWasDown)
                {
                    m_graphics
                        .DecreaseBloomThreshold();
                }

                bloomThresholdDecreaseWasDown =
                    bloomThresholdDecreaseIsDown;


                const bool bloomThresholdIncreaseIsDown =
                    m_window.IsKeyDown(VK_OEM_7);

                if (
                    bloomThresholdIncreaseIsDown &&
                    !bloomThresholdIncreaseWasDown)
                {
                    m_graphics
                        .IncreaseBloomThreshold();
                }

                bloomThresholdIncreaseWasDown =
                    bloomThresholdIncreaseIsDown;


                //
                // - / = = bloom exposure down / up.
                //
                const bool bloomExposureDecreaseIsDown =
                    m_window.IsKeyDown(VK_OEM_MINUS);

                if (
                    bloomExposureDecreaseIsDown &&
                    !bloomExposureDecreaseWasDown)
                {
                    m_graphics
                        .DecreaseExposure();
                }

                bloomExposureDecreaseWasDown =
                    bloomExposureDecreaseIsDown;


                const bool bloomExposureIncreaseIsDown =
                    m_window.IsKeyDown(VK_OEM_PLUS);

                if (
                    bloomExposureIncreaseIsDown &&
                    !bloomExposureIncreaseWasDown)
                {
                    m_graphics
                        .IncreaseExposure();
                }

                bloomExposureIncreaseWasDown =
                    bloomExposureIncreaseIsDown;

                constexpr float
                    coreIntensityAdjustmentPerSecond =
                    2.0f;

                constexpr float
                    exposureAdjustmentPerSecond =
                    1.0f;

                constexpr float
                    galaxySpinAdjustmentPerSecond =
                    0.30f;

                constexpr float
                    spiralSpreadAdjustmentPerSecond =
                    0.45f;

                constexpr float
                    armConcentrationAdjustmentPerSecond =
                    1.40f;

                constexpr float
                    colorGradientAdjustmentPerSecond =
                    0.35f;

                constexpr float
                    particleSpeedAdjustmentPerSecond =
                    0.75f;


                //
                // 4 / 5 = galaxy spin down / up.
                //
                // Continuous control, not discrete.
                //
                const float galaxySpinInput =
                    static_cast<float>(
                        m_window.IsKeyDown('5')
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown('4')
                        );

                m_graphics
                    .AdjustGalaxySpin(
                        galaxySpinInput *
                        galaxySpinAdjustmentPerSecond *
                        controlDeltaSeconds
                    );


                //
                // 2 / 3 = particle-core HDR intensity
                // down / up.
                //
                const float coreIntensityInput =
                    static_cast<float>(
                        m_window.IsKeyDown('3')
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown('2')
                        );


                m_graphics
                    .AdjustCoreParticleIntensity(
                        coreIntensityInput*
                        coreIntensityAdjustmentPerSecond*
                        controlDeltaSeconds
                    );

                //
                // 6 / 7 = exposure down / up
                //
                const float exposureInput =
                    static_cast<float>(
                        m_window.IsKeyDown('7')
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown('6')
                        );

                m_graphics
                    .AdjustExposure(
                        exposureInput *
                        exposureAdjustmentPerSecond *
                        controlDeltaSeconds
                    );


                //
                // 9 / 0 = differential rotation down / up.
                //
                // These are intentionally discrete presses.
                //
                const bool differentialDownIsDown =
                    m_window.IsKeyDown('9');

                const bool differentialUpIsDown =
                    m_window.IsKeyDown('0');


                if (
                    differentialDownIsDown &&
                    !differentialDownWasDown)
                {
                    m_graphics
                        .AdjustDifferentialRotationStrength(
                            -0.05f
                        );
                }


                if (
                    differentialUpIsDown &&
                    !differentialUpWasDown)
                {
                    m_graphics
                        .AdjustDifferentialRotationStrength(
                            +0.05f
                        );
                }


                differentialDownWasDown =
                    differentialDownIsDown;

                differentialUpWasDown =
                    differentialUpIsDown;

                //
                // Z / X = spiral winding down / up.
                //
                const float spiralInput =
                    static_cast<float>(
                        m_window.IsKeyDown('X')
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown('Z')
                        );

                m_graphics
                    .AdjustSpiralStrength(
                        spiralInput *
                        spiralSpreadAdjustmentPerSecond *
                        controlDeltaSeconds
                    );

                //
                // N / M = arm concentration down / up.
                //
                const float armConcentrationInput =
                    static_cast<float>(
                        m_window.IsKeyDown('M')
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown('N')
                        );

                m_graphics
                    .AdjustArmConcentration(
                        armConcentrationInput *
                        armConcentrationAdjustmentPerSecond *
                        controlDeltaSeconds
                    );

                //
                // F2 / F3 = particle motion speed down / up.
                // Continuous control, not discrete.
                //
                const float particleSpeedInput =
                    static_cast<float>(
                        m_window.IsKeyDown(VK_F3)
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown(VK_F2)
                        );

                m_graphics
                    .AdjustParticleMotionSpeed(
                        particleSpeedInput *
                        particleSpeedAdjustmentPerSecond *
                        controlDeltaSeconds
                    );

                //
                // , / . = color gradient down / up.
                //
                const float colorGradientInput =
                    static_cast<float>(
                        m_window.IsKeyDown(VK_OEM_PERIOD)
                        )
                    -
                    static_cast<float>(
                        m_window.IsKeyDown(VK_OEM_COMMA)
                        );

                m_graphics
                    .AdjustColorGradient(
                        colorGradientInput *
                        colorGradientAdjustmentPerSecond *
                        controlDeltaSeconds
                    );


                m_graphics.Resize(
                    m_window.ClientWidth(),
                    m_window.ClientHeight()
                );

                m_graphics.Render(
                    elapsedSeconds,
                    deltaSeconds,
                    m_camera
                );
            }
            else
            {
                std::this_thread::sleep_for(16ms);
            }

            if (currentTime >= nextStatusUpdate)
            {
                const auto diagnostics =
                    m_graphics.GetFrameDiagnostics();

                const auto selectedGravityPosition =
                    m_graphics
                    .SelectedInteractiveGravitySourcePosition();

                const auto selectedGravitySource =
                    m_graphics
                    .SelectedInteractiveGravitySourceIndex();

                const auto activeGravitySources =
                    m_graphics
                    .InteractiveGravitySourceCount();

                const std::string graphicsStatus =
                    std::format(
                        "F{} FR{} BB{} fence {}/{} | "
                        "D:{} core:{:.1f} exp:{:.1f} | "
                        "bloom:{} thr:{:.2f} int:{:.2f} | "
                        "sim:{} p:{} pd:{} "
                        "spin:{:.2f} diff:{:.2f} "
                        "sp:{:.2f} arm:{:.1f} col:{:.2f} "
                        "den:{:.0f}% speed:{:.2f} "
                        "rp:{:.2f} vf:{:.2f} draw:{} | "
                        "cam:{} {} | "
                        "gs:{} {}/{} "
                        "@({:.1f},{:.1f},{:.1f})",

                        diagnostics.submittedFrameCount,
                        diagnostics.nextFrameResourceIndex,
                        diagnostics.nextBackBufferIndex,
                        diagnostics.completedFenceValue,
                        diagnostics.lastSubmittedFenceValue,

                        m_graphics.IsDepthTestingEnabled()
                        ? "on"
                        : "off",

                        m_graphics.CoreParticleIntensity(),
                        m_graphics.Exposure(),

                        m_graphics.IsBloomEnabled()
                        ? "on"
                        : "off",

                        m_graphics.BloomThreshold(),
                        m_graphics.BloomIntensity(),

                        m_graphics.IsParticleSimulationEnabled()
                        ? "run"
                        : "pause",

                        m_graphics.IsParticleRenderingEnabled()
                        ? "on"
                        : "off",

                        m_graphics.IsParticleDepthTestingEnabled()
                        ? "on"
                        : "off",

                        m_graphics.GalaxySpin(),
                        m_graphics.DifferentialRotationStrength(),
                        m_graphics.SpiralSpread(),
                        m_graphics.ArmConcentration(),
                        m_graphics.ColorGradientExponent(),
                        m_graphics.ParticleDensity() * 100.0f,
                        m_graphics.ParticleMotionSpeed(),
                        m_graphics.RadialPerturbationStrength(),
                        m_graphics.VerticalOscillationFrequency(),
                        m_graphics.ParticleDebugDrawCount(),

                        cameraPresetIndex + 1,
                        cameraPresets[
                            static_cast<std::size_t>(
                                cameraPresetIndex
                            )
                        ].label,

                        m_graphics.IsInteractiveGravitySourceEnabled()
                        ? "on"
                        : "off",

                        selectedGravitySource + 1,
                        activeGravitySources,

                        selectedGravityPosition.x,
                        selectedGravityPosition.y,
                        selectedGravityPosition.z
                    );
                m_window.UpdateStatusTitle(
                    currentTime - startTime,
                    std::wstring(
                        graphicsStatus.begin(),
                        graphicsStatus.end()
                    )
                );

                nextStatusUpdate =
                    currentTime + 175ms;
            }
        }

        core::LogInfo("Application stopped");
        return 0;
    }
}
