#ifndef __MATERIALDESC_H__
#define __MATERIALDESC_H__

struct MaterialDesc
{
	float4 Color; //.a = brightness

	float Opacity;
	float SpecularPower;

	uint Type;

	float pad;

	uint TextureId;
	uint AlbedoId;
	uint SpecularId;
	uint MetallicId;

};

//Material Types
#define MAT_TYPE_DEFAULT 0u
#define MAT_TYPE_TRANSLUCENT 1u
#define MAT_TYPE_TWOSIDED 2u
#define MAT_TYPE_EMITTER 4u

#endif