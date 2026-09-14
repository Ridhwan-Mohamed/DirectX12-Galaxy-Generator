#pragma once

#include <cstdint>
#include <numbers>
#include <algorithm>
#include <array>
#include <cmath>

namespace gf::simulation
{
    struct GalaxyParameters final
    {
        //
        // Number of coherent spiral arms.
        //
        std::uint32_t armCount = 7;


        //
        // Pitch angle of the underlying
        // logarithmic spiral, in radians.
        //
        float armPitch =
            0.0f *
            std::numbers::pi_v<float> /
            180.0f;

        //
        // Controls how strongly particles cluster
        // around each spiral-arm centreline.
        //
        // 1 = uniform throughout the arm's sector.
        // Larger values increasingly concentrate
        // particles near the centreline.
        //
        float armConcentration = 1.3f;

        //
        // Fraction of arm particles deliberately
        // distributed uniformly through their
        // angular sector.
        //
        // This provides a faint inter-arm population
        // while preserving strongly defined arms.
        //
        float interArmFraction = 0.f;        

        //
        // Radius-normalized arm-envelope tuning.
        //
        // At inner radius, regular arm particles may
        // occupy this half-angle around the spiral
        // centreline (in degrees).
        //
        float regularArmHalfWidthInnerDegrees = 52.0f;

        //
        // At outer radius, regular arm particles may
        // occupy this half-angle around the spiral
        // centreline (in degrees).
        //
        float regularArmHalfWidthOuterDegrees = 8.0f;

        //
        // Curve shape for the regular-arm half-width
        // reduction with radius.
        //
        // 0 = almost linear reduction in the inner disk,
        // larger = tighter collapse towards the outside.
        //
        float regularArmHalfWidthRadiusPower = 0.95f;

        //
        // Outer-lean bias for regular-arm width.
        // This increases envelope tightening in the
        // outer disk while preserving inner spread.
        //
        // 0 = no outer-lean.
        //
        float regularArmHalfWidthOuterBias = 0.40f;

        //
        // Radius-normalized inter-arm fraction tuning.
        //
        // Fraction of disk particles that become
        // inter-arm particles at inner radius.
        //
        float interArmFractionInner = 0.30f;

        //
        // Fraction of disk particles that become
        // inter-arm particles at outer radius.
        //
        float interArmFractionOuter = 0.22f;

        //
        // Curve shape for inter-arm fraction with radius.
        //
        // 1 = near-linear decay.
        //
        float interArmFractionRadiusPower = 1.0f;

        //
        // Outer-lean bias for inter-arm probability.
        // This helps preserve a stronger arm-only
        // morphology in the outer disk.
        //
        // 0 = no extra outer-lean.
        //
        float interArmFractionOuterBias = 0.33f;

        //
        // Multiplier applied to spiral winding.
        //
        // 0 = radial spokes
        // 1 = full logarithmic spiral
        //
        float spiralSpread = 1.0f;

        //
        // Controls how aggressively core
        // particles concentrate toward r = 0.
        //
        // Larger = more centrally concentrated.
        //
        float coreFalloff = 3.0f;

        //
        // Base angular rotation rate,
        // in radians per second.
        //
        float spin = 0.75f;

        //
        // Radius at which the rotation curve begins
        // transitioning away from near-rigid motion.
        //
        float rotationCurveRadius = 1.25f;

        //
        // 0 = rigid angular rotation.
        // 1 = full differential rotation curve.
        //
        float differentialRotationStrength = 0.18f;

        //
        // Fraction of normal orbital acceleration
        // used for gentle radial perturbation.
        //
        float radialPerturbationStrength = 0.08f;


        //
        // Radial perturbation frequency relative
        // to the local orbital angular velocity.
        //
        float radialPerturbationFrequencyScale = 1.25f;


        //
        // Damp persistent radial drift without
        // eliminating the oscillation.
        //
        float radialVelocityDamping = 0.12f;


        //
        // Base vertical oscillation frequency
        // in radians per second.
        //
        float verticalOscillationFrequency = 0.90f;


        //
        // Spatial parameters needed by the
        // population stages that follow.
        //
        float innerArmRadius = 0.25f;
        float outerArmRadius = 2.30f;
        float coreRadius = 0.70f;
        //
        // Fraction of the total particle population
        // assigned to the galactic core.
        //
        float coreParticleFraction = 0.34f;


        //
        // Vertical half-thickness of the central
        // stellar bulge.
        //
        float coreHalfThickness = 0.22f;

        //
        // Maximum billboard-size multiplier
        // for particles at the center of the core.
        //
        float coreParticleSizeMultiplier = 1.75f;


        //
        // Maximum HDR intensity multiplier
        // for particles at the center of the core.
        //
        float coreParticleIntensity = 3.0f;

        float diskHalfThickness = 0.10f;

        //
        // Stellar colour at the galactic centre.
        //
        std::array<float, 3> coreColor{
            1.00f,
            0.78f,
            0.52f
        };

        //
        // Stellar colour toward the outer disk.
        //
        std::array<float, 3> outerArmColor{
            0.48f,
            0.68f,
            1.00f
        };

        //
        // Shapes the radial warm -> cool transition.
        //
        // 1 = linear.
        // < 1 becomes cool earlier.
        // > 1 retains warm tones farther outward.
        //
        float colorGradientExponent = 0.85f;

        //
        // Maximum per-particle perturbation along
        // the warm/cool temperature axis.
        //
        float colorJitterStrength = 0.07f;

        //
        // Relative brightness of stars at the
        // outer edge of the galactic disk.
        //
        // 1 = no radial dimming.
        //
        float outerArmIntensity = 0.55f;


        //
        // Shapes radial brightness falloff.
        //
        // 1 = linear.
        // > 1 keeps the inner disk brighter longer.
        // < 1 dims earlier.
        //
        float brightnessFalloffExponent = 1.35f;

        //
        // Maximum deterministic per-star intensity
        // variation.
        //
        // 0 = identical stars.
        // 0.15 = approximately +/- 15%.
        //
        float stellarIntensityJitter = 0.15f;

        float starParticleFraction = 0.02f;

        float starRotationSpeed = 0.0f;

        float starPulseAmplitude = 0.18f;

        float starPulseFrequency = 0.80f;

        //
        // HDR emission multiplier applied only
        // to selected star particles.
        //
        // 1.0 = same HDR brightness as normal particles.
        // >1.0 = hotter stars and therefore stronger bloom.
        //
        float starHdrIntensity = 4.0f;
    };


    inline constexpr GalaxyParameters
        DefaultGalaxyParameters{};


    [[nodiscard]]
    inline float ComputeSpiralAngleAtRadius(
        const GalaxyParameters& parameters,
        float radius
    ) noexcept
    {
        constexpr float minimumRadius =
            0.0001f;

        constexpr float minimumPitchTangent =
            0.0001f;


        const float safeInnerRadius =
            std::max(
                parameters.innerArmRadius,
                minimumRadius
            );


        const float safeRadius =
            std::max(
                radius,
                safeInnerRadius
            );


        const float pitchTangent =
            std::tan(
                parameters.armPitch
            );


        //
        // A zero pitch would require division
        // by zero. Treat an effectively-zero
        // pitch as no spiral winding.
        //
        if (
            std::abs(pitchTangent) <
            minimumPitchTangent
            )
        {
            return 0.0f;
        }


        return
            parameters.spiralSpread *
            std::log(
                safeRadius /
                safeInnerRadius
            ) /
            pitchTangent;
    }


    [[nodiscard]]
    inline float ComputeArmPhase(
        const GalaxyParameters& parameters,
        std::uint32_t armIndex
    ) noexcept
    {
        constexpr float twoPi =
            2.0f *
            std::numbers::pi_v<float>;


        const std::uint32_t safeArmCount =
            std::max(
                parameters.armCount,
                1u
            );


        const std::uint32_t wrappedArmIndex =
            armIndex %
            safeArmCount;


        return
            twoPi *
            (
                static_cast<float>(
                    wrappedArmIndex
                    ) /
                static_cast<float>(
                    safeArmCount
                    )
                );
    }


    [[nodiscard]]
    inline float ComputeSpiralArmAngle(
        const GalaxyParameters& parameters,
        std::uint32_t armIndex,
        float radius
    ) noexcept
    {
        return
            ComputeArmPhase(
                parameters,
                armIndex
            ) +
            ComputeSpiralAngleAtRadius(
                parameters,
                radius
            );
    }

    [[nodiscard]]
    inline float ComputeGalaxyAngularVelocityAtRadius(
        const GalaxyParameters& parameters,
        float radius
    ) noexcept
    {
        const float safeRadius =
            std::max(
                radius,
                0.0f
            );


        const float safeRotationCurveRadius =
            std::max(
                parameters.rotationCurveRadius,
                0.0001f
            );


        const float normalizedRadius =
            safeRadius /
            safeRotationCurveRadius;


        const float differentialCurveScale =
            1.0f /
            std::sqrt(
                1.0f +
                normalizedRadius *
                normalizedRadius
            );


        const float differentialStrength =
            std::clamp(
                parameters
                .differentialRotationStrength,
                0.0f,
                1.0f
            );


        const float angularVelocityScale =
            (
                1.0f -
                differentialStrength
                )
            +
            differentialStrength *
            differentialCurveScale;


        return
            parameters.spin *
            angularVelocityScale;
    }
}
