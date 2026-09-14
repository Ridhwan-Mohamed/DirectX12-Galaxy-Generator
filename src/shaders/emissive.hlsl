cbuffer SceneConstants : register(b0)
{
    row_major float4x4 modelViewProjection;
    float4 colorTint;
    float2 materialParameters;
    float2 materialPadding;
    float4 animationParameters;
};

Texture2D<float4> baseColorTexture : register(t0);

SamplerState linearWrapSampler : register(s0);

struct PixelInput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PixelInput input) : SV_Target0
{
    const float elapsedSeconds =
        animationParameters.x;

    const float emissiveIntensity =
        max(
            animationParameters.y,
            0.0f
        );

    const float pulseSpeed =
        max(
            animationParameters.z,
            0.0f
        );

    const float4 sampledColor =
        baseColorTexture.Sample(
            linearWrapSampler,
            input.uv
        );

    const float twoPi = 6.28318530718f;

    const float primaryPulse =
        0.68f +
        0.32f *
        sin(
            elapsedSeconds *
            pulseSpeed *
            twoPi
        );

    const float energyBands =
        0.5f +
        0.5f *
        sin(
            input.uv.y *
            twoPi *
            5.0f -
            elapsedSeconds *
            pulseSpeed *
            3.0f
        );

    const float rotatingDetail =
        0.5f +
        0.5f *
        sin(
            input.uv.x *
            twoPi *
            8.0f +
            elapsedSeconds *
            pulseSpeed *
            2.0f
        );

    const float textureLuminance =
        dot(
            sampledColor.rgb,
            float3(
                0.2126f,
                0.7152f,
                0.0722f
            )
        );

    const float textureDetail =
        lerp(
            0.80f,
            1.20f,
            textureLuminance
        );

    const float animatedDetail =
        lerp(
            0.82f,
            1.18f,
            energyBands *
            rotatingDetail
        );

    const float emissionStrength =
        emissiveIntensity *
        primaryPulse *
        textureDetail *
        animatedDetail;

    const float3 emission =
        colorTint.rgb *
        emissionStrength;

    return float4(
        emission,
        1.0f
    );
}