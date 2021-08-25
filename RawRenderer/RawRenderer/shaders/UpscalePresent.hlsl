
#include "Utility.hlsli"

struct Interpolators
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

void MainVS(
	in uint VertID : SV_VertexID,
	out Interpolators VSout
)
{
	VSout.TexCoord = float2(uint2(VertID, VertID << 1) & 2);
	VSout.Position = float4(lerp(float2(-1, 1), float2(1, -1), VSout.TexCoord), 0, 1);
}

Texture2D ColorTexture : register(t0);

SamplerState BilinearFilter : register(s0);

float4 MainPS(Interpolators PSin) : SV_Target
{
	//#if DEFAULT_UPSCALE
	//	float3 LinearRGB = RemoveDisplayProfile(ColorTexture[(int2)Interpolators.Position.xy], COLOR_FORMAT);
	//	return ApplyDisplayProfile(LinearRGB, DISPLAY_PLANE_FORMAT);
	//#endif

#if BILLINEAR_UPSCALE
		return ColorTexture.SampleLevel(BilinearFilter, PSin.TexCoord, 0);

#else //DEFAULT POINT
		return ColorTexture[(int2)PSin.Position.xy];
#endif

}