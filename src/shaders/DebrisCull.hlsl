#include "DebrisCommon.hlsli"

cbuffer DebrisCullingConstants : register(b0)
{
    float4 frustumPlanes[6];

    float animationSeconds;
    uint instanceCount;
    float frustumInset;
    float padding;
};

StructuredBuffer<DebrisInstance>
    debrisInstances : register(t0);

RWStructuredBuffer<uint>
    visibilityFlags : register(u0);

RWStructuredBuffer<uint>
    visibleInstanceIndices : register(u1);

RWBuffer<uint>
    visibleCount : register(u2);

[numthreads(64, 1, 1)]
void CSMain(
    uint3 dispatchThreadId
        : SV_DispatchThreadID)
{
    const uint instanceId =
        dispatchThreadId.x;

    if (instanceId >= instanceCount)
    {
        return;
    }

    const DebrisInstance instance =
        debrisInstances[instanceId];

    const float3 center =
        DebrisWorldCenter(
            instance,
            animationSeconds
        );

    const float radius =
        DebrisWorldBoundRadius(
            instance
        );

    uint isVisible = 1;

    [unroll]
    for (
        uint planeIndex = 0;
        planeIndex < 6;
        ++planeIndex)
    {
        const float signedDistance =
            dot(
                float4(center, 1.0f),
                frustumPlanes[planeIndex]
            );

        if (
            signedDistance <
            frustumInset - radius
        )
        {
            isVisible = 0;
            break;
        }
    }

    visibilityFlags[instanceId] =
    isVisible;

    if (isVisible != 0)
    {
        uint visibleSlot;

        InterlockedAdd(
        visibleCount[0],
        1,
        visibleSlot
    );

        visibleInstanceIndices[visibleSlot] =
        instanceId;
    }
}
