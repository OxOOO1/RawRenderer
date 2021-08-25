
#include "UniformBuffer.hlsli"
#include "SimpleGeometryBuffer.hlsli"

struct DebugDrawInstanceDesc
{
	matrix ModelMatrix;
	float4 Color;
}

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


////////////////////
//Bound per-Draw
/////////////////////

//All

//Vertex
StructuredBuffer<DebugDrawInstanceDesc> AllInstancesDescs : register (t0);

//Pixel


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
	nointerpolation float3 Color : COLOR 
};


void MainVS(
	in VertexId : SV_VertexID,
	in InstanceId : SV_InstanceID,
	out Interpolators VSout
	)
{
	
	DebugDrawInstanceDesc Desc = AllInstancesDescs[InstanceId];

	float4 Position = GeometryBuffer[VertexId];
	Position = mul(Position, Desc.ModelMatrix);
	VSout.Position = mul(Position, GView.ProjectionMatrix);

	VSout.Color = Desc.Color;

}

//===========================================================================
//
//								PIXEL SHADER
//
//===========================================================================


float4 MainPS(Interpolators PSin) : SV_Target
{
	return float4(PSin.Color.rgb, 1.f);
}