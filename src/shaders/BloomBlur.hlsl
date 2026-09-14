Texture2D<float4>
    sourceTexture : register(t0);

SamplerState
    linearClampSampler : register(s0);


cbuffer BloomBlurConstants : register(b0)
{
    float2 texelSize;
    float2 blurDirection;
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


float4 PSMain(
    VertexOutput input) : SV_Target0
{
    //
    // 9-tap separable Gaussian.
    //
    // These weights sum to approximately 1.0 when
    // symmetric samples on both sides are included.
    //
    static const float weights[5] =
    {
        0.2270270270f,
        0.1945945946f,
        0.1216216216f,
        0.0540540541f,
        0.0162162162f
    };


    const float blurRadius = 1.25f;

    const float2 sampleStep =
    texelSize *
    blurDirection *
    blurRadius;


    float3 result =
        sourceTexture.Sample(
            linearClampSampler,
            input.uv
        ).rgb *
        weights[0];


    [unroll]
    for (uint sampleIndex = 1;
         sampleIndex < 5;
         ++sampleIndex)
    {
        const float2 offset =
            sampleStep *
            float(sampleIndex);

        result +=
            sourceTexture.Sample(
                linearClampSampler,
                input.uv + offset
            ).rgb *
            weights[sampleIndex];

        result +=
            sourceTexture.Sample(
                linearClampSampler,
                input.uv - offset
            ).rgb *
            weights[sampleIndex];
    }


    return float4(
        result,
        1.0f
    );
}