Texture2D<float4>
    hdrSceneTexture : register(t0);

Texture2D<float4>
    bloomTexture : register(t1);

SamplerState
    linearClampSampler : register(s0);

cbuffer PresentationConstants : register(b0)
{
    float exposure;
    float bloomIntensity;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOutput VSMain(
    uint vertexId : SV_VertexID)
{
    VertexOutput output;

    const float2 uv =
        float2(
            (vertexId << 1) & 2,
            vertexId & 2
        );

    output.position =
        float4(
            uv *
            float2(
                2.0f,
                -2.0f
            )
            +
            float2(
                -1.0f,
                1.0f
            ),
            0.0f,
            1.0f
        );

    output.uv = uv;

    return output;
}

float3 ToneMap(
    float3 hdrColor)
{
    const float3 nonNegative =
        max(
            hdrColor,
            0.0f
        );

    // ACES-like filmic mapping.
    const float3 acesA = 2.51f;
    const float3 acesB = 0.03f;
    const float3 acesC = 2.43f;
    const float3 acesD = 0.59f;
    const float3 acesE = 0.14f;

    const float3 mapped =
        (nonNegative * (acesA * nonNegative + acesB)) /
        (nonNegative * (acesC * nonNegative + acesD) + acesE);

    // Approximate sRGB OETF for display output.
    return pow(
        saturate(mapped),
        1.0f / 2.2f
    );
}

float4 PSMain(
    VertexOutput input) : SV_Target0
{
    const float3 hdrColor =
        max(
            hdrSceneTexture.Sample(
                linearClampSampler,
                input.uv
            ).rgb,
            0.0f
        );

    const float3 bloomColor =
        max(
            bloomTexture.Sample(
                linearClampSampler,
                input.uv
            ).rgb,
            0.0f
        );

    const float safeBloomIntensity =
        max(
            bloomIntensity,
            0.0f
        );

    //
    // Compose while still in HDR space.
    //
    const float3 composedHdr =
        hdrColor +
        bloomColor *
        safeBloomIntensity;

    const float safeExposure =
        max(
            exposure,
            0.0f
        );

    const float3 exposedColor =
        composedHdr *
        safeExposure;

    const float3 displayColor =
        ToneMap(
            exposedColor
        );

    return float4(
        displayColor,
        1.0f
    );
}
