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

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(
        float4(input.position, 1.0f),
        modelViewProjection
    );

    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    const float4 sampledColor =
        baseColorTexture.Sample(
            linearWrapSampler,
            input.uv
        );

    const float metallic =
        saturate(materialParameters.x);

    const float roughness =
        saturate(materialParameters.y);

    const float3 baseColor =
        sampledColor.rgb * colorTint.rgb;

    // Temporary UV-space inspection light.
    // This makes material parameters visible before
    // we have normals and a real lighting model.
    const float2 highlightOffset =
        input.uv - float2(0.35f, 0.30f);

    const float highlightBase =
        saturate(
            1.0f -
            dot(
                highlightOffset,
                highlightOffset
            ) * 6.0f
        );

    const float highlightExponent =
        lerp(
            32.0f,
            2.0f,
            roughness
        );

    const float highlight =
        pow(
            highlightBase,
            highlightExponent
        );

    const float3 dielectricSpecular =
        float3(
            0.04f,
            0.04f,
            0.04f
        );

    const float3 specularColor =
        lerp(
            dielectricSpecular,
            baseColor,
            metallic
        );

    const float diffuseStrength =
        lerp(
            1.0f,
            0.35f,
            metallic
        );

    const float3 finalColor =
        baseColor * diffuseStrength +
        specularColor * highlight;

    return float4(
        finalColor,
        sampledColor.a * colorTint.a
    );
}