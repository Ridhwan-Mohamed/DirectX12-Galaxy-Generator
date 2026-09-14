#ifndef GRAVITY_FORGE_DEBRIS_COMMON_HLSLI
#define GRAVITY_FORGE_DEBRIS_COMMON_HLSLI

static const float DebrisTwoPi =
    6.28318530718f;

struct DebrisInstance
{
	float4 orbit;
	float4 orbitOrientation;
	float4 localOrientation;
	float4 scaleAndBoundRadius;
	float4 tumble;
};

float3 RotateByQuaternion(
    float3 value,
    float4 quaternion)
{
	return value +
        2.0f *
        cross(
            quaternion.xyz,
            cross(
                quaternion.xyz,
                value
            ) +
            quaternion.w * value
        );
}

float4 AxisAngleQuaternion(
    float3 axis,
    float angle)
{
	const float halfAngle =
        0.5f * angle;

	float sine;
	float cosine;

	sincos(
        halfAngle,
        sine,
        cosine
    );

	return float4(
        axis * sine,
        cosine
    );
}

float DebrisOrbitAngle(
    DebrisInstance instance,
    float animationSeconds)
{
	return
        instance.orbit.y +
        fmod(
            animationSeconds *
                instance.orbit.z,
            DebrisTwoPi
        );
}

float3 DebrisWorldCenter(
    DebrisInstance instance,
    float animationSeconds)
{
	const float angle =
        DebrisOrbitAngle(
            instance,
            animationSeconds
        );

	const float3 orbitPosition =
        float3(
            cos(angle) *
                instance.orbit.x,

            instance.orbit.w,

            sin(angle) *
                instance.orbit.x
        );

	return RotateByQuaternion(
        orbitPosition,
        instance.orbitOrientation
    );
}

float DebrisWorldBoundRadius(
    DebrisInstance instance)
{
	return
        instance.scaleAndBoundRadius.w;
}

#endif