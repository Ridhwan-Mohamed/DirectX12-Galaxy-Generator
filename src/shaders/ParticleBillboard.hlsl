struct ParticleState
{
    float3 position;
    float age;

    float3 velocity;
    float seed;
};


cbuffer ParticleRenderConstants : register(b0)
{
    row_major float4x4 viewProjection;

    float3 cameraRight;
    float particleHalfSize;

    float3 cameraUp;
    float elapsedSeconds;

    float4 particleColor;

    //
    // x = core radius
    // y = core particle fraction
    // z = center size multiplier
    // w = center intensity multiplier
    //
    float4 coreParameters;
    
    //
    // xyz = warm core colour.
    //
    float4 coreColor;


    //
    // xyz = cool outer-arm colour.
    //
    float4 outerArmColor;


    //
    // x = outer arm radius
    // y = colour-gradient exponent
    // z = colour-jitter strength
    // w = unused
    //
    float4 colorParameters;
    
    //
    // x = outer-arm intensity
    // y = brightness-falloff exponent
    // z = stellar intensity jitter
    // w = selected-star HDR emission multiplier
    //
    float4 toneParameters;
    
    float4 starParameters;
};

StructuredBuffer<ParticleState>
    particleState : register(t0);


static const float2 BillboardCorners[6] =
{
    float2(-1.0f, -1.0f),
    float2(-1.0f, 1.0f),
    float2(1.0f, 1.0f),

    float2(-1.0f, -1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, -1.0f)
};

//
// Selected stars need a complete square billboard.
//
// Normal particles continue using BillboardCorners,
// so their geometry does not change.
//
static const float2 StarBillboardCorners[6] =
{
    //
    // Triangle 1
    //
    float2(-1.0f, -1.0f),
    float2(-1.0f, 1.0f),
    float2(1.0f, 1.0f),

    //
    // Triangle 2
    //
    float2(-1.0f, -1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, -1.0f)
};

static const float TwoPi =
    6.28318530718f;

float Hash01(float value)
{
    return
        frac(
            sin(
                value * 12.9898f +
                78.233f
            ) *
            43758.5453f
        );
}

bool IsStarParticle(
    ParticleState particle)
{
    const float selectionSample =
        Hash01(
            particle.seed *
            2137.119f +
            31.731f
        );

    return
        selectionSample <
        saturate(
            starParameters.x
        );
}

float2 RotateBillboardCoordinate(
    float2 coordinate,
    float angle)
{
    const float sine =
        sin(angle);

    const float cosine =
        cos(angle);


    return float2(
        coordinate.x * cosine -
        coordinate.y * sine,

        coordinate.x * sine +
        coordinate.y * cosine
    );
}

float ComputeStarPulse(
    ParticleState particle)
{
    //
    // Give every star a deterministic
    // starting phase.
    //
    // Hash01 gives:
    //
    //     0 -> 1
    //
    // multiplying by TwoPi gives:
    //
    //     0 -> 2PI
    //
    const float phase =
        Hash01(
            particle.seed *
            5987.531f +
            19.417f
        ) *
        TwoPi;


    //
    // Give every star a slightly different
    // pulse frequency.
    //
    const float frequencySample =
        Hash01(
            particle.seed *
            7213.937f +
            47.113f
        );


    //
    // Frequency varies between:
    //
    //     75% -> 125%
    //
    // of the configured base frequency.
    //
    const float frequencyScale =
        lerp(
            0.75f,
            1.25f,
            frequencySample
        );


    //
    // Prevent negative configuration values
    // from doing anything strange.
    //
    const float pulseAmplitude =
    clamp(
        starParameters.z,
        0.0f,
        0.95f
    );


    const float pulseFrequency =
        max(
            starParameters.w,
            0.0f
        );


    //
    // Standard sine-wave pulse:
    //
    //     1 + A * sin(...)
    //
    // The 1.0 means the normal particle size
    // remains the centre of the pulse.
    //
    return
        1.0f
        +
        pulseAmplitude *
        sin(
            elapsedSeconds *
            pulseFrequency *
            frequencyScale *
            TwoPi
            +
            phase
        );
}

float ComputeStarRotation(
    ParticleState particle)
{
    //
    // Give every star a deterministic
    // initial orientation from 0 -> 2PI.
    //
    const float initialRotationSample =
        Hash01(
            particle.seed *
            3251.713f +
            11.971f
        );


    const float initialAngle =
        initialRotationSample *
        TwoPi;


    //
    // Separate hash from star selection
    // and initial rotation.
    //
    // Remap:
    //
    //     Hash01:       0 -> 1
    //
    // into:
    //
    //     speedSample: -1 -> 1
    //
    const float speedSample =
        Hash01(
            particle.seed *
            4721.337f +
            73.193f
        ) *
        2.0f -
        1.0f;


    //
    // Negative stars rotate one way.
    // Positive stars rotate the other way.
    //
    const float rotationDirection =
        speedSample < 0.0f
        ? -1.0f
        : 1.0f;


    //
    // Do not let some stars become
    // practically stationary.
    //
    // Every selected star rotates at between
    // 45% and 100% of the configured speed.
    //
    const float rotationSpeedScale =
        lerp(
            0.45f,
            1.0f,
            abs(speedSample)
        );


    const float baseRotationSpeed =
        max(
            starParameters.y,
            0.0f
        );


    //
    // angle(t) =
    //
    // initialAngle
    // +
    // time * angularVelocity
    //
    return
        initialAngle
        +
        elapsedSeconds *
        baseRotationSpeed *
        rotationSpeedScale *
        rotationDirection;
}

float ComputeNormalizedGalaxyRadius(
    float3 particlePosition)
{
    const float planarRadius =
        length(
            particlePosition.xz
        );


    const float safeCoreRadius =
        max(
            coreParameters.x,
            0.0001f
        );


    const float safeOuterRadius =
        max(
            colorParameters.x,
            safeCoreRadius +
            0.0001f
        );


    return
        saturate(
            (
                planarRadius -
                safeCoreRadius
            )
            /
            (
                safeOuterRadius -
                safeCoreRadius
            )
        );
}

float3 HsvToRgb(
    float3 hsv)
{
    const float hue =
        hsv.x;

    const float saturation =
        hsv.y;

    const float value =
        hsv.z;


    const float4 k =
        float4(
            1.0f,
            2.0f / 3.0f,
            1.0f / 3.0f,
            3.0f
        );


    const float3 p =
        abs(
            frac(
                hue.xxx +
                k.xyz
            ) *
            6.0f -
            k.www
        );


    return
        value *
        lerp(
            k.xxx,
            saturate(
                p - k.xxx
            ),
            saturation
        );
}

float3 ComputeStarColor(
    ParticleState particle)
{
    //
    // Deterministic random hue:
    // 0 -> 1 maps around the whole color wheel.
    //
    const float hue =
        Hash01(
            particle.seed *
            8429.371f +
            61.773f
        );


    //
    // Keep saturation fairly high so colors
    // are vivid and noticeable.
    //
    const float saturation =
        lerp(
            0.55f,
            0.95f,
            Hash01(
                particle.seed *
                9311.743f +
                27.419f
            )
        );


    //
    // Keep value bright so stars don't become
    // muddy or too dark.
    //
    const float value =
        lerp(
            0.90f,
            1.00f,
            Hash01(
                particle.seed *
                6151.227f +
                93.117f
            )
        );


    return
        HsvToRgb(
            float3(
                hue,
                saturation,
                value
            )
        );
}

float3 ComputeParticleTemperatureColor(
    ParticleState particle)
{
    float temperatureCoordinate =
    ComputeNormalizedGalaxyRadius(
        particle.position
    );


    //
    // Convert world-space radius into
    // a normalized temperature coordinate:
    //
    // core radius  -> 0
    // outer radius -> 1
    //
    //
    // Shape how quickly the galaxy changes
    // from warm to cool.
    //
    const float safeGradientExponent =
        max(
            colorParameters.y,
            0.01f
        );


    temperatureCoordinate =
        pow(
            temperatureCoordinate,
            safeGradientExponent
        );


    //
    // Small deterministic temperature jitter.
    //
    // This moves particles ALONG the same
    // warm/cool palette rather than creating
    // arbitrary RGB colours.
    //
    const float jitterSample =
        Hash01(
            particle.seed *
            937.731f +
            17.117f
        );


    const float signedJitter =
        jitterSample *
        2.0f -
        1.0f;


    const float jitterStrength =
        saturate(
            colorParameters.z
        );


    temperatureCoordinate =
        saturate(
            temperatureCoordinate +
            signedJitter *
            jitterStrength
        );


    return
        lerp(
            coreColor.rgb,
            outerArmColor.rgb,
            temperatureCoordinate
        );
}

float ComputeParticleRadialIntensity(
    ParticleState particle)
{
    float radialCoordinate =
        ComputeNormalizedGalaxyRadius(
            particle.position
        );


    const float safeFalloffExponent =
        max(
            toneParameters.y,
            0.01f
        );


    radialCoordinate =
        pow(
            radialCoordinate,
            safeFalloffExponent
        );


    const float outerIntensity =
        saturate(
            toneParameters.x
        );


    return
        lerp(
            1.0f,
            outerIntensity,
            radialCoordinate
        );
}

float ComputeParticleStellarIntensity(
    ParticleState particle)
{
    const float jitterSample =
        Hash01(
            particle.seed *
            1571.913f +
            41.731f
        );


    const float signedJitter =
        jitterSample *
        2.0f -
        1.0f;


    const float jitterStrength =
        saturate(
            toneParameters.z
        );


    return
        max(
            1.0f +
            signedJitter *
            jitterStrength,
            0.0f
        );
}

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;

    // Position inside this billboard.
    //
    // (-1,-1) -------- (1,-1)
    //    |                |
    //    |      0,0       |
    //    |                |
    // (-1, 1) -------- (1, 1)
    //
    float2 billboardPosition : TEXCOORD0;
    nointerpolation uint isStar : TEXCOORD1;
};

bool IsCoreParticle(
    uint instanceId,
    float coreParticleFraction)
{
    const float populationSample =
        frac(
            (float) instanceId *
            0.754877666f
        );

    return
        populationSample <
        saturate(
            coreParticleFraction
        );
}

VertexOutput VSMain(
    uint vertexId : SV_VertexID,
    uint instanceId : SV_InstanceID)
{
    VertexOutput output;


    const ParticleState particle =
        particleState[instanceId];
    
    const bool isCoreParticle =
    IsCoreParticle(
        instanceId,
        coreParameters.y
    );


    const float safeCoreRadius =
    max(
        coreParameters.x,
        0.0001f
    );


    const float planarRadius =
    length(
        particle.position.xz
    );
    
    const float normalizedCoreRadius =
    saturate(
        planarRadius /
        safeCoreRadius
    );
    
    const float centerWeight =
        1.0f - normalizedCoreRadius;


    const float coreVisualWeight =
        isCoreParticle ? centerWeight * centerWeight : 0.0f;

    const bool isStarParticle =
    IsStarParticle(
        particle
    );


    //
    // Normal particle:
    //     use the original diamond.
    //
    // Selected star:
    //     use a full square support billboard.
    //
    const float2 localCorner =
    isStarParticle
    ? StarBillboardCorners[vertexId]
    : BillboardCorners[vertexId];


    float2 worldCorner =
    localCorner;


    if (isStarParticle)
    {
        const float starRotation =
        ComputeStarRotation(
            particle
        );


        worldCorner =
        RotateBillboardCoordinate(
            localCorner,
            starRotation
        );
    }
    
    const float safeCoreSizeMultiplier =
        max(
            coreParameters.z,
            1.0f
        );

    float billboardHalfSize =
    particleHalfSize *
    lerp(
        1.0f,
        safeCoreSizeMultiplier,
        coreVisualWeight
    );


    if (isStarParticle)
    {
        const float starBaseSizeMultiplier =
        1.5f;

        billboardHalfSize *=
        starBaseSizeMultiplier *
        ComputeStarPulse(
            particle
        );
    }

    const float3 worldPosition =
        particle.position
        +
        cameraRight *
        worldCorner.x *
        billboardHalfSize
        +
        cameraUp *
        worldCorner.y *
        billboardHalfSize;
    
    const float safeCoreIntensity =
        max(
            coreParameters.w,
            1.0f
        );


    const float coreIntensity =
    lerp(
        1.0f,
        safeCoreIntensity,
        coreVisualWeight
    );
    
    const float radialIntensity =
    ComputeParticleRadialIntensity(
        particle
    );


    const float stellarIntensity =
    ComputeParticleStellarIntensity(
        particle
    );


    //
    // Selected stars are allowed to emit
    // substantially more HDR energy.
    //
    // Normal particles always use 1.0,
    // so their brightness does not change.
    //
    const float starHdrIntensity =
    isStarParticle
    ? max(
        toneParameters.w,
        1.0f
    )
    : 1.0f;


    const float particleIntensity =
    coreIntensity *
    radialIntensity *
    stellarIntensity *
    starHdrIntensity;

    output.position =
        mul(
            float4(
                worldPosition,
                1.0f
            ),
            viewProjection
        );

    const float3 particleTint =
    ComputeParticleTemperatureColor(
        particle
    );

    float3 finalParticleColor =
    particleTint *
    particleColor.rgb;


    if (isStarParticle)
    {
        const float3 starColor =
        ComputeStarColor(
            particle
        );


        finalParticleColor =
        lerp(
            finalParticleColor,
            starColor,
            0.75f
        );
    }


    output.color =
    float4(
        finalParticleColor *
        particleIntensity,
        particleColor.a
    );

    output.billboardPosition =
        localCorner;

    output.isStar =
        isStarParticle
        ? 1u
        : 0u;

    return output;
}

float ComputeStarParticleIntensity(
    float2 billboardPosition)
{
    //
    // Mirror all four quadrants into
    // the positive quadrant.
    //
    const float2 p =
        abs(
            billboardPosition
        );


    //
    // The four outer points sit at:
    //
    //     ( 1, 0)
    //     (-1, 0)
    //     ( 0, 1)
    //     ( 0,-1)
    //
    // innerRadius controls where the
    // concave corners occur diagonally.
    //
    // Smaller = sharper/thinner star.
    // Larger = fuller star.
    //
    const float innerRadius =
        0.30f;


    //
    // For innerRadius = 0.30:
    //
    //     slope ~= 2.333
    //
    // This produces straight edges from
    // each outer point into the diagonal
    // inner corners.
    //
    const float sideSlope =
        (
            1.0f -
            innerRadius
        )
        /
        innerRadius;


    //
    // Fold the quadrant so one equation
    // describes all eight sides of the star.
    //
    const float majorAxis =
        max(
            p.x,
            p.y
        );


    const float minorAxis =
        min(
            p.x,
            p.y
        );


    //
    // <= 1.0 means inside the star.
    //
    //
    // Along an axis:
    //
    //     minorAxis = 0
    //
    // so the star reaches 1.0.
    //
    // Along the diagonal:
    //
    //     majorAxis == minorAxis
    //
    // and the boundary occurs at
    // approximately innerRadius.
    //
    const float starDistance =
        majorAxis
        +
        sideSlope *
        minorAxis;


    //
    // Soft edge for antialiasing /
    // additive particle rendering.
    //
    const float edgeFeather =
        0.045f;


    const float starCoverage =
        1.0f -
        smoothstep(
            1.0f -
            edgeFeather,
            1.0f,
            starDistance
        );


    //
    // Keep a bright stellar centre.
    //
    const float radiusSquared =
        dot(
            billboardPosition,
            billboardPosition
        );


    const float radialFalloff =
        saturate(
            1.0f -
            radiusSquared
        );


    const float centerGlow =
        radialFalloff *
        radialFalloff;


    //
    // Don't completely kill the four tips.
    //
    // Center stays bright while the
    // pointed rays remain visible.
    //
    const float brightness =
        lerp(
            0.25f,
            1.0f,
            centerGlow
        );


    return
        starCoverage *
        brightness;
}

float ComputeNormalParticleIntensity(
    float2 billboardPosition)
{
    //
    // Preserve the original particle
    // shaping calculation.
    //
    const float radiusSquared =
        dot(
            billboardPosition,
            billboardPosition
        );


    const float radialFalloff =
        saturate(
            1.0f -
            radiusSquared
        );


    return
        radialFalloff *
        radialFalloff;
}

float4 PSMain(
    VertexOutput input) : SV_Target0
{
    //
    // Normal particles use the exact
    // original radial shaping.
    //
    // Only selected stars use the
    // procedural four-point shape.
    //
    const float intensity =
        input.isStar != 0u
        ? ComputeStarParticleIntensity(
            input.billboardPosition
        )
        : ComputeNormalParticleIntensity(
            input.billboardPosition
        );


    return float4(
        input.color.rgb *
        intensity,
        0.0f
    );
}