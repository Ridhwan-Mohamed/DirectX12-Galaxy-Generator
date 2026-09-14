Texture2D<float4>
    hdrSceneTexture : register(t0);

SamplerState
    linearClampSampler : register(s0);

cbuffer BloomExtractConstants : register(b0)
{
    float bloomThreshold;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};


//
// Same fullscreen-triangle pattern already used by
// HdrPresent.hlsl.
//
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

    //
    // Rec.709 luminance.
    //
    const float luminance =
        dot(
            hdrColor,
            float3(
                0.2126f,
                0.7152f,
                0.0722f
            )
        );

    const float safeThreshold =
        max(
            bloomThreshold,
            0.0f
        );

    //
    // P1 is deliberately a simple hard threshold.
    //
    // Later, if necessary, we can introduce a soft knee
    // without changing the resource architecture.
    //
    const float keep =
        luminance > safeThreshold
        ? 1.0f
        : 0.0f;

    return float4(
        hdrColor * keep,
        1.0f
    );
}