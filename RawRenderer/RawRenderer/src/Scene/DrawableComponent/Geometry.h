#pragma once

#include <Windows.h>
#include "Vertex.h"
#include "Node.h"
#include "Resources/GpuBuffer.h"


using namespace DirectX;

struct RMeshDesc
{
	UINT FirstIndexPos = 0;
	UINT FirstVertexPos = 0;
	UINT NumofIndices = 0;
	UINT NumOfVertices = 0;
};

struct RIndexBufferCPU
{
	std::vector<unsigned int> Indices;
};

//Geometry Buffer to Draw Meshes
class RGeometryBufferCPU 
{
public:

	//void Init(const RVertexAttribute::EAttribType* attributes, UINT numAttributes)
	//{
	//	//VertexBufferCPU.Init(attributes, numAttributes);
	//}
	
	//If the buffer represents one Mesh
	RMeshDesc GetMeshDesc()
	{
		RMeshDesc md;
		md.FirstIndexPos = 0;
		md.FirstVertexPos = 0;
		md.NumofIndices = IndexBufferCPU.Indices.size();
		md.NumOfVertices = VertexBufferPositionsCPU.GetNumVertices();
		return md;
	}

	std::vector<RMeshDesc> PerLODMeshDescs;

	RVertexBufferCPU VertexBufferPositionsCPU;
	RVertexBufferCPU VertexBufferRestCPU;
	RIndexBufferCPU IndexBufferCPU;


};
