#ifndef __LIGHTDESC_H__
#define __LIGHTDESC_H__

struct LightDesc
{
	float3 Position;
	float Radius;

	float3 Color;
	uint Type;

	float3 Direction;
	float Intensity;

	float2 SpotlightAngles;

	float2 pad;
};

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1

#endif