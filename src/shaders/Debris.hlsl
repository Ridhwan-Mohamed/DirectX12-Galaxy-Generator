#include "DebrisCommon.hlsli"

cbuffer SceneConstants : register(b0)
{
    row_major float4x4 viewProjection;
    float4 colorTint;
    float2 materialParameters;
    float2 materialPadding;
    float4 animationParameters;
};

StructuredBuffer<DebrisInstance>
    debrisInstances : register(t0, space1);

StructuredBuffer<uint>
    visibleInstanceIndices
        : register(t1, space1);

Buffer<uint>
    visibleCount
        : register(t2, space1);

StructuredBuffer<uint>
    visibilityFlags
        : register(t3, space1);

Texture2D<float4>
    baseColorTexture : register(t0);

SamplerState
    linearWrapSampler : register(s0);

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

VertexOutput VSMain(
    VertexInput input,
    uint instanceId : SV_InstanceID)
{
    VertexOutput output;

    const uint renderMode =
        (uint)round(animationParameters.y);

    static const uint CulledMode = 1;
    static const uint CullingDebugMode = 2;

    uint sourceInstanceId = instanceId;

    // M13 can make the compact count drive an
    // indirect draw. M12 clips the unused tail of
    // this single direct instanced draw instead.
    if (renderMode == CulledMode)
    {
        if (instanceId >= visibleCount[0])
        {
            // Direct3D clip-space depth begins at 0.
            output.position =
                float4(0.0f, 0.0f, -1.0f, 1.0f);

            output.color =
                float3(0.0f, 0.0f, 0.0f);

            output.uv =
                float2(0.0f, 0.0f);

            return output;
        }

        sourceInstanceId =
            visibleInstanceIndices[instanceId];
    }

    const DebrisInstance instance =
        debrisInstances[sourceInstanceId];

    const float animationSeconds =
        animationParameters.x;

    const float3 center =
        DebrisWorldCenter(
            instance,
            animationSeconds
        );

    float3 worldPosition =
        input.position *
        instance.scaleAndBoundRadius.xyz;

    worldPosition = RotateByQuaternion(
        worldPosition,
        instance.localOrientation
    );

    const float tumbleAngle =
        fmod(
            animationSeconds *
                instance.tumble.w,
            DebrisTwoPi
        );

    const float4 tumbleOrientation =
        AxisAngleQuaternion(
            instance.tumble.xyz,
            tumbleAngle
        );

    worldPosition = RotateByQuaternion(
        worldPosition,
        tumbleOrientation
    );

    worldPosition += center;

    output.position = mul(
        float4(worldPosition, 1.0f),
        viewProjection
    );

    if (renderMode == CullingDebugMode)
    {
        const bool accepted =
            visibilityFlags[sourceInstanceId] != 0;

        output.color =
            accepted
            ? float3(0.10f, 1.50f, 0.15f)
            : float3(1.50f, 0.08f, 0.03f);
    }
    else
    {
        output.color =
            float3(1.0f, 1.0f, 1.0f);
    }

    output.uv = input.uv;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    const uint renderMode =
        (uint)round(animationParameters.y);

    if (renderMode == 2)
    {
        return float4(input.color, 1.0f);
    }

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
        float3(0.04f, 0.04f, 0.04f);

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
