
#include "UniformBuffer.hlsli"
#include "Lights.hlsli"
#include "Material.hlsli"
#include "VertexInput.h"

//===========================================================================
//								BASE PASS ROOT SIG
//===========================================================================

///////////////////////////
//Bound during whole Pass
///////////////////////////

//All
//Uniform Buffer : b0

//Vertex

//Pixel
StructuredBuffer<LightDesc> LightsBuffer : register(t0);
StructuredBuffer<MaterialDesc> MaterialDescsBuffer : register (t1);

Texture2D InputTexture[] : register(t5);
SamplerState DefaultSampler : register(s0);

////////////////////
//Bound per-Draw
/////////////////////

//All

//Vertex
StructuredBuffer<VertexInputRest> VertexBufferRest : register (t0);
StructuredBuffer<matrix> AllInstancesModelMatrices : register (t1);
ByteAddressBuffer AllInstancesIndicesToMaterials : register(t2);

//Pixel
cbuffer RootConstants : register(b1)
{
	uint val1;
	uint val2;
	uint val3;
	uint val4;
}

//===========================================================================
//
//								VERTEX SHADER
//
//===========================================================================

//Functions

//Declarations
struct Interpolators
{
	float4 Position : SV_Position;
	float3 PositionWorld : POSWORLD;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
	nointerpolation uint MaterialId : MATERIALID;
};


void MainVS(
	in float3 position : Position,
	in uint InstanceId : SV_InstanceID,
	in uint VertexId : SV_VertexID,
	out Interpolators VSout
	)
{

	matrix ModelMatrix = AllInstancesModelMatrices[InstanceId];

	//Positions
	float4 worldPos = mul(float4(position, 1.0f), ModelMatrix);
	VSout.PositionWorld = worldPos.xyz;
	worldPos = mul(worldPos, GView.ViewMatrix);
	VSout.Position = mul(float4(worldPos.xyz, 1), GView.ProjectionMatrix);

	//Rest
	VertexInputRest inputRest = VertexBufferRest[VertexId]; //TODO: Its IndexID, getVertexId based on Index

	VSout.Normal = mul(inputRest.Normal, (float3x3) ModelMatrix);

	VSout.TexCoord = inputRest.TexCoord;

	VSout.MaterialId = AllInstancesIndicesToMaterials.Load(InstanceId * 4);
}

//===========================================================================
//
//								PIXEL SHADER
//
//===========================================================================

float4 MainPS(Interpolators PSin) : SV_Target
{
	MaterialDesc MatDesc = MaterialDescsBuffer[PSin.MaterialId];

	float3 SceneColor = float3(0,0,0);
	float3 ambientColor = float3(0.1, 0.1, 0.1);

	float3 normal = normalize(PSin.Normal);
	//return InputTexture[MatDesc.TextureId].SampleLevel(DefaultSampler, PSin.TexCoord, 0);
	return float4(MatDesc.Color.rgb * MatDesc.Color.a, 1.f);
	//
	if(val1)
	{
		clip(MatDesc.Color.r - 0.3);
	}
	// if(MatDesc.Type & MAT_TYPE_EMITTER)
	// {
	// 	return float4(MatDesc.Color.rgb * MatDesc.Color.a, 1.f);
	// }
	// else
	// {
	// 	SceneColor +=  MatDesc.Color.rgb * ambientColor;

	// 	for(uint i = 0; i < GNumLocalLights; i++)
	// 	{
	// 		SceneColor += CalculatePointLight(LightsBuffer[i], PSin.PositionWorld, MatDesc.Color.rgb, normal, MatDesc.SpecularPower);
	// 	}
	// }

	
	return float4(SceneColor, 1.f);

}