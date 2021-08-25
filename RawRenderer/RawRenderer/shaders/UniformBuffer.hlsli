
#ifndef __UNIFORMBUFFER_HLSLI__
#define __UNIFORMBUFFER_HLSLI__

struct View
{
	matrix ViewMatrix;
	matrix ProjectionMatrix;
	matrix ViewProjectionMatrix;
};

struct GlobalLightDesc
{
	float3 DirectionViewSpace;
	float Intensity;
	float3 Color;
	float AmbientLightIntensity;
};


cbuffer ViewUniformBuffer : register(b0)
{
	View GView;

	GlobalLightDesc GGlobalLight;

	float3 GCameraPosition;

	uint GNumLocalLights;
}

#endif // __UNIFORMBUFFER_HLSLI__