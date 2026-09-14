struct ParticleState
{
    float3 position;
    float age;

    float3 velocity;
    float seed;
};


cbuffer SceneConstants : register(b0)
{
    row_major float4x4 modelViewProjection;

    float4 colorTint;

    float2 materialParameters;
    float2 materialPadding;

    float4 animationParameters;
};


StructuredBuffer<ParticleState>
    particleState : register(t0);


struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};


VertexOutput VSMain(
    uint vertexId : SV_VertexID)
{
    VertexOutput output;

    const ParticleState particle =
        particleState[vertexId];

    output.position =
        mul(
            float4(
                particle.position,
                1.0f
            ),
            modelViewProjection
        );

    output.color =
        colorTint;

    return output;
}


float4 PSMain(
    VertexOutput input) : SV_Target0
{
    return input.color;
}