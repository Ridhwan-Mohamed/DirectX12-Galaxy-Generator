struct ParticleState
{
    float3 position;
    float age;

    float3 velocity;
    float seed;
};

StructuredBuffer<ParticleState>
    previousState : register(t0);

RWStructuredBuffer<ParticleState>
    nextState : register(u0);

static const uint
    MaxInteractiveGravitySources =
    3;

static const uint
    SimulationModeOrbit =
    0;

static const uint
    SimulationModeExplosion =
    1;

static const uint
    SimulationModeReform =
    2;

struct GravitySource
{
    float3 position;
    float strength;
};

struct ReformTarget
{
    float3 position;
    float3 velocity;
};

cbuffer SimulationConstants : register(b0)
{
    //
    // Basic simulation.
    //
    float deltaTime;
    float simulationPadding0;
    float softeningSquared;
    uint particleCount;


    //
    // Small bounded array of
    // user-controlled forces.
    //
    GravitySource
        interactiveGravitySources[
            MaxInteractiveGravitySources
        ];


    //
    // How many entries above
    // should actually contribute?
    //
    uint interactiveGravitySourceCount;

    uint simulationMode;

    float explosionStrength;
    float explosionTangentialStrength;
    
    float reformPositionStrength;
    float reformVelocityStrength;
    float reformMaxAcceleration;
    float reformPadding;
    
    //
    // Galaxy topology.
    //
    uint galaxyArmCount;
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

float3 ComputeGravityAcceleration(
    float3 particlePosition,
    float3 sourcePosition,
    float sourceStrength)
{
    const float3 offsetToSource =
        sourcePosition -
        particlePosition;


    const float distanceSquared =
        dot(
            offsetToSource,
            offsetToSource
        );


    const float minimumDistanceSquared =
        max(
            softeningSquared,
            0.000001f
        );


    const float safeDistanceSquared =
        max(
            distanceSquared,
            minimumDistanceSquared
        );


    const float inverseSafeDistance =
        rsqrt(
            safeDistanceSquared
        );


    const float3 directionToSource =
        offsetToSource *
        inverseSafeDistance;


    const float safeStrength =
        max(
            sourceStrength,
            0.0f
        );


    const float accelerationMagnitude =
        safeStrength /
        safeDistanceSquared;


    return
        directionToSource *
        accelerationMagnitude;
}

float3 ComputeExplosionImpulse(
    ParticleState particle)
{
    //
    // Direction straight outward
    // from the reactor.
    //
    const float radiusSquared =
        dot(
            particle.position,
            particle.position
        );


    float3 outwardDirection;


    //
    // Normally position gives us
    // the radial direction.
    //
    if (radiusSquared > 0.000001f)
    {
        outwardDirection =
            particle.position *
            rsqrt(radiusSquared);
    }
    else
    {
        //
        // Extremely defensive fallback:
        // if a particle sits almost exactly
        // at the origin, derive a direction
        // from its deterministic seed.
        //
        const float angle =
            particle.seed *
            6.28318530718f;

        outwardDirection =
            normalize(
                float3(
                    cos(angle),
                    0.25f,
                    sin(angle)
                )
            );
    }


    //
    // Build a direction perpendicular
    // to the radial direction.
    //
    float3 tangentDirection =
        cross(
            float3(
                0.0f,
                1.0f,
                0.0f
            ),
            outwardDirection
        );


    float tangentLengthSquared =
        dot(
            tangentDirection,
            tangentDirection
        );


    //
    // If radial direction happens to
    // align with world-up, use another
    // axis to build the tangent.
    //
    if (tangentLengthSquared <
        0.000001f)
    {
        tangentDirection =
            cross(
                float3(
                    1.0f,
                    0.0f,
                    0.0f
                ),
                outwardDirection
            );

        tangentLengthSquared =
            dot(
                tangentDirection,
                tangentDirection
            );
    }


    tangentDirection *=
        rsqrt(
            max(
                tangentLengthSquared,
                0.000001f
            )
        );


    //
    // Half the particles twist one way,
    // half twist the other.
    //
    const float tangentSign =
        particle.seed < 0.5f
        ? -1.0f
        : 1.0f;


    //
    // Slight per-particle radial variation.
    //
    const float radialVariation =
        0.80f +
        0.40f *
        frac(
            particle.seed *
            7.913f
        );


    const float3 radialImpulse =
        outwardDirection *
        explosionStrength *
        radialVariation;


    const float3 tangentialImpulse =
        tangentDirection *
        explosionTangentialStrength *
        tangentSign;


    return
        radialImpulse +
        tangentialImpulse;
}

uint HashParticleValue(
    uint value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;

    value ^= value >> 15;
    value *= 0x846ca68bu;

    value ^= value >> 16;

    return value;
}


float HashParticle01(
    uint value)
{
    static const float
        Inverse24BitRange =
        1.0f /
        16777216.0f;

    return
        (float) (
            HashParticleValue(value) >>
            8
        ) *
        Inverse24BitRange;
}

float ComputeGalaxySpiralAngleAtRadius(
    float radius)
{
    const float safeInnerRadius =
        max(
            galaxyInnerArmRadius,
            0.0001f
        );


    const float safeRadius =
        max(
            radius,
            safeInnerRadius
        );


    const float pitchTangent =
        tan(
            galaxyArmPitch
        );


    if (
        abs(pitchTangent) <
        0.0001f)
    {
        return 0.0f;
    }


    return
        galaxySpiralSpread *
        log(
            safeRadius /
            safeInnerRadius
        ) /
        pitchTangent;
}


float ComputeGalaxyArmPhase(
    uint armIndex)
{
    static const float TwoPi =
        6.28318530718f;


    const uint safeArmCount =
        max(
            galaxyArmCount,
            1u
        );


    return
        TwoPi *
        (
            (float) (
                armIndex %
                safeArmCount
            ) /
            (float) safeArmCount
        );
}


float ComputeGalaxyArmAngle(
    uint armIndex,
    float radius)
{
    return
        ComputeGalaxyArmPhase(
            armIndex
        )
        +
        ComputeGalaxySpiralAngleAtRadius(
            radius
        );
}

bool IsGalaxyCoreParticle(
    uint particleIndex)
{
    const float populationSample =
        frac(
            (float) particleIndex *
            0.754877666f
        );


    return
        populationSample <
        saturate(
            galaxyCoreParticleFraction
        );
}

float ComputeGalaxyAngularVelocityAtRadius(
    float radius)
{
    const float safeRadius =
        max(
            radius,
            0.0f
        );


    const float safeRotationCurveRadius =
        max(
            galaxyRotationCurveRadius,
            0.0001f
        );


    const float normalizedRadius =
        safeRadius /
        safeRotationCurveRadius;


    const float differentialCurveScale =
        1.0f /
        sqrt(
            1.0f +
            normalizedRadius *
            normalizedRadius
        );


    const float differentialStrength =
        saturate(
            galaxyDifferentialRotationStrength
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
        galaxySpin *
        angularVelocityScale;
}

ReformTarget ComputeGalaxyCoreTarget(
    uint particleIndex,
    float particleAge)
{
    static const float TwoPi =
        6.28318530718f;

    static const float GoldenAngle =
        2.39996322972865332f;


    const float index =
        (float) particleIndex;


    const float radialSample =
        frac(
            index *
            0.569840291f +
            0.417f
        );


    const float safeCoreFalloff =
        max(
            galaxyCoreFalloff,
            0.001f
        );


    const float radius =
        galaxyCoreRadius *
        pow(
            radialSample,
            safeCoreFalloff
        );


    const float baseAngle =
        fmod(
            index *
            GoldenAngle,
            TwoPi
        );


    const float heightSample =
        frac(
            index *
            0.438579021f +
            0.271f
        );


    const float normalizedRadius =
        galaxyCoreRadius >
        0.0001f
        ? radius /
          galaxyCoreRadius
        : 0.0f;


    const float heightEnvelope =
        sqrt(
            max(
                0.0f,
                1.0f -
                normalizedRadius *
                normalizedRadius
            )
        );


    const float height =
        (
            heightSample *
            2.0f -
            1.0f
        ) *
        galaxyCoreHalfThickness *
        heightEnvelope;


    const float angularSpeed =
        ComputeGalaxyAngularVelocityAtRadius(
            radius
        );


    const float orbitSpeed =
        radius *
        angularSpeed;


    const float angle =
        fmod(
            baseAngle +
            particleAge *
            angularSpeed,
            TwoPi
        );


    const float cosAngle =
        cos(angle);

    const float sinAngle =
        sin(angle);


    ReformTarget target;


    target.position =
        float3(
            cosAngle * radius,
            height,
            sinAngle * radius
        );


    target.velocity =
        float3(
            -sinAngle *
            orbitSpeed,
            0.0f,
            cosAngle *
            orbitSpeed
        );


    return target;
}

float3 ComputeGalaxyOrbitalAcceleration(
    float3 particlePosition)
{
    const float radius =
        length(
            particlePosition.xz
        );


    if (
        radius <
        0.0001f)
    {
        return
            float3(
                0.0f,
                0.0f,
                0.0f
            );
    }


    const float angularVelocity =
        ComputeGalaxyAngularVelocityAtRadius(
            radius
        );


    const float angularVelocitySquared =
        angularVelocity *
        angularVelocity;


    return
        float3(
            -particlePosition.x *
                angularVelocitySquared,

            0.0f,

            -particlePosition.z *
                angularVelocitySquared
        );
}

float3 ComputeGalaxyPerturbationAcceleration(
    ParticleState particle)
{
    static const float TwoPi =
        6.28318530718f;


    const float radius =
        length(
            particle.position.xz
        );


    if (
        radius <
        0.0001f)
    {
        return
            float3(
                0.0f,
                0.0f,
                0.0f
            );
    }


    //
    // Current radial direction in the
    // galactic plane.
    //
    const float3 radialDirection =
        float3(
            particle.position.x / radius,
            0.0f,
            particle.position.z / radius
        );


    const float angularVelocity =
        ComputeGalaxyAngularVelocityAtRadius(
            radius
        );


    //
    // Normal centripetal acceleration
    // magnitude:
    //
    //     a = r * omega^2
    //
    const float orbitalAccelerationMagnitude =
        radius *
        angularVelocity *
        angularVelocity;


    //
    // Give each particle a deterministic
    // perturbation phase.
    //
    const float radialPhase =
        TwoPi *
        particle.seed;


    //
    // Slight per-particle variation keeps
    // everything from breathing in sync.
    //
    const float frequencyVariation =
        0.80f +
        0.40f *
        frac(
            particle.seed *
            11.713f
        );

    const float radialFrequency =
        angularVelocity *
        max(
            galaxyRadialPerturbationFrequencyScale,
            0.0f
        ) *
        frequencyVariation;

    const float radialOscillation =
        sin(
            particle.age *
            radialFrequency +
            radialPhase
        );


    const float perturbationStrength =
        max(
            galaxyRadialPerturbationStrength,
            0.0f
        );


    //
    // Zero-mean radial perturbation.
    //
    float radialAcceleration =
        orbitalAccelerationMagnitude *
        perturbationStrength *
        radialOscillation;


    //
    // Remove persistent inward/outward drift.
    //
    const float radialVelocity =
        dot(
            particle.velocity,
            radialDirection
        );


    radialAcceleration -=
        radialVelocity *
        max(
            galaxyRadialVelocityDamping,
            0.0f
        );


    //
    // Vertical harmonic oscillator.
    //
    const float verticalFrequencyVariation =
        0.85f +
        0.30f *
        frac(
            particle.seed *
            19.371f
        );


    const float verticalFrequency =
        max(
            galaxyVerticalOscillationFrequency,
            0.0f
        ) *
        verticalFrequencyVariation;


    const float verticalAcceleration =
        -particle.position.y *
        verticalFrequency *
        verticalFrequency;


    return
        radialDirection *
        radialAcceleration
        +
        float3(
            0.0f,
            verticalAcceleration,
            0.0f
        );
}

ReformTarget ComputeGalaxyArmTarget(
    uint particleIndex,
    float particleAge)
{
    static const float Pi =
        3.14159265359f;

    static const float TwoPi =
        6.28318530718f;


    const uint safeArmCount =
        max(
            galaxyArmCount,
            1u
        );


    const uint armIndex =
        particleIndex %
        safeArmCount;


    //
    // Channel 0:
    // radial location.
    //
    const float radialSample =
        HashParticle01(
            particleIndex *
            6u +
            0u
        );


    const float safeInnerRadius =
        max(
            galaxyInnerArmRadius,
            0.0001f
        );


    const float safeOuterRadius =
        max(
            galaxyOuterArmRadius,
            safeInnerRadius
        );


    const float baseRadius =
        safeInnerRadius +
        (
            safeOuterRadius -
            safeInnerRadius
        ) *
        radialSample;

    const float radialT =
        saturate(
            (baseRadius - safeInnerRadius) /
            max(
                safeOuterRadius -
                safeInnerRadius,
                0.0001f
            )
        );

    const float radius =
        clamp(
            baseRadius,
            safeInnerRadius,
            safeOuterRadius
        );


    const float armCenterAngle =
        ComputeGalaxyArmAngle(
            armIndex,
            radius
        );


    //
    // Each arm owns one complete angular
    // sector surrounding its centerline.
    //
    const float halfSectorAngle =
        Pi /
        (float) safeArmCount;


    //
    // Channel 1:
    // signed position through the sector.
    //
    const float angularSample =
        HashParticle01(
            particleIndex *
            6u +
            1u
        );


    const float signedAngularSample =
        angularSample *
        2.0f -
        1.0f;


    const float angularSign =
        signedAngularSample <
        0.0f
        ? -1.0f
        : 1.0f;


    //
    // Channel 2:
    // choose concentrated arm particle
    // or diffuse inter-arm particle.
    //
    const float populationStyleSample =
        HashParticle01(
            particleIndex *
            6u +
            2u
        );


    const bool isInterArmParticle =
        populationStyleSample <
        saturate(
            lerp(
                galaxyInterArmFractionInner,
                            galaxyInterArmFractionOuter,
                            pow(
                                saturate(
                                    (baseRadius -
                                        safeInnerRadius) /
                                    max(
                                        safeOuterRadius -
                                        safeInnerRadius,
                            0.0001f
                        ) *
                        (1.0f +
                            max(
                                galaxyInterArmFractionOuterBias,
                                0.0f
                            ))
                    ),
                    max(
                        galaxyInterArmFractionRadiusPower,
                        0.0001f
                    )
                )
            )
        );

    const float regularArmHalfSectorAngle =
        min(
            radians(
                galaxyRegularArmHalfWidthInnerDegrees
            ) *
            (1.0f - radialT * radialT),
            halfSectorAngle
        );

    const float interArmGapHalfAngle =
        max(
            halfSectorAngle -
            regularArmHalfSectorAngle,
            0.0f
        );


    const float normalizedAngularOffset =
        isInterArmParticle
        ? angularSign *
            (regularArmHalfSectorAngle +
            abs(
                signedAngularSample
            ) *
            interArmGapHalfAngle)
        : signedAngularSample *
            regularArmHalfSectorAngle;


    const float angularArmOffset =
        normalizedAngularOffset;


    const float baseAngle =
        armCenterAngle +
        angularArmOffset;


    //
    // P4 differential rotation.
    //
    const float angularSpeed =
        ComputeGalaxyAngularVelocityAtRadius(
            radius
        );


    const float angle =
        fmod(
            baseAngle +
            particleAge *
            angularSpeed,
            TwoPi
        );


    const float cosAngle =
        cos(angle);

    const float sinAngle =
        sin(angle);


    //
    // Channels 3 + 4:
    // triangular vertical distribution.
    //
    const float heightSampleA =
        HashParticle01(
            particleIndex *
            6u +
            3u
        );


    const float heightSampleB =
        HashParticle01(
            particleIndex *
            6u +
            4u
        );


    const float centeredHeightSample =
        heightSampleA +
        heightSampleB -
        1.0f;


    const float height =
        centeredHeightSample *
        galaxyDiskHalfThickness;


    //
    // v = r * omega
    //
    const float orbitSpeed =
        radius *
        angularSpeed;


    ReformTarget target;


    target.position =
        float3(
            cosAngle * radius,
            height,
            sinAngle * radius
        );


    target.velocity =
        float3(
            -sinAngle *
            orbitSpeed,
            0.0f,
            cosAngle *
            orbitSpeed
        );


    return target;
}

ReformTarget ComputeReformTarget(
    uint particleIndex,
    float particleAge)
{
    if (
        IsGalaxyCoreParticle(
            particleIndex
        )
        )
    {
        return
            ComputeGalaxyCoreTarget(
                particleIndex,
                particleAge
            );
    }


    return
        ComputeGalaxyArmTarget(
            particleIndex,
            particleAge
        );
}

float3 ComputeReformAcceleration(
    ParticleState particle,
    ReformTarget target)
{
    const float3 positionError =
        target.position -
        particle.position;


    const float3 velocityError =
        target.velocity -
        particle.velocity;


    //
    // Spring:
    //
    // position term pulls us home.
    //
    // velocity term removes explosion
    // velocity and matches orbit motion.
    //
    float3 acceleration =
        positionError *
        reformPositionStrength
        +
        velocityError *
        reformVelocityStrength;


    //
    // Don't let a particle very far away
    // receive an absurd acceleration.
    //
    const float accelerationSquared =
        dot(
            acceleration,
            acceleration
        );


    const float safeMaximumAcceleration =
        max(
            reformMaxAcceleration,
            0.001f
        );


    const float
        maximumAccelerationSquared =
        safeMaximumAcceleration *
        safeMaximumAcceleration;


    if (
        accelerationSquared >
        maximumAccelerationSquared)
    {
        acceleration *=
            safeMaximumAcceleration *
            rsqrt(
                accelerationSquared
            );
    }


    return acceleration;
}

[numthreads(256, 1, 1)]
void CSMain(
    uint3 dispatchThreadId :
    SV_DispatchThreadID)
{
    const uint particleIndex =
        dispatchThreadId.x;

    if (particleIndex >= particleCount)
    {
        return;
    }


    ParticleState particle =
        previousState[particleIndex];

    const float safeDeltaTime =
        max(
            deltaTime,
            0.0f
        );
    
    float3 acceleration =
    float3(
        0.0f,
        0.0f,
        0.0f
    );


    if (
    simulationMode ==
    SimulationModeReform)
    {
    //
    // During reform the reform controller
    // exclusively owns particle acceleration.
    //
        const ReformTarget target =
        ComputeReformTarget(
            particleIndex,
            particle.age
        );


        acceleration =
        ComputeReformAcceleration(
            particle,
            target
        );
    }
    else
    {
    //
    // Explosion is a one-shot impulse.
    //
        if (
        simulationMode ==
        SimulationModeExplosion)
        {
            particle.velocity +=
            ComputeExplosionImpulse(
                particle
            );
        }


    //
    // Normal galactic orbital acceleration.
    //
    acceleration =
        ComputeGalaxyOrbitalAcceleration(
            particle.position
        )
        +
        ComputeGalaxyPerturbationAcceleration(
            particle
        );


    //
    // Optional interactive gravity sources.
    //
    const uint activeSourceCount =
        interactiveGravitySourceCount <
        MaxInteractiveGravitySources
        ? interactiveGravitySourceCount
        : MaxInteractiveGravitySources;


    [unroll]
        for (
        uint sourceIndex = 0;
        sourceIndex <
            MaxInteractiveGravitySources;
        ++sourceIndex)
        {
            if (
            sourceIndex <
            activeSourceCount)
            {
                const GravitySource source =
                interactiveGravitySources[
                    sourceIndex
                ];


                acceleration +=
                ComputeGravityAcceleration(
                    particle.position,
                    source.position,
                    source.strength
                );
            }
        }
    
    }

    //
    // IMPORTANT:
    // Integration happens for ALL simulation modes.
    //
    particle.velocity +=
    acceleration *
    safeDeltaTime;


    particle.position +=
    particle.velocity *
    safeDeltaTime;


    particle.age +=
    safeDeltaTime;


    nextState[particleIndex] =
    particle;

}
