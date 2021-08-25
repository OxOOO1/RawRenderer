#pragma once

#include "Utilities/Types.h"
#include <string>
#include "Resources/GpuBuffer.h"

class RGeometryBufferCPU;

//Data needed to Draw
class RDrawDataGPU
{
public:

	void CreateVertexIndexBuffers(RGeometryBufferCPU& mesh);

	void CreateInstanceBuffers(const std::string& name, UINT NumInstances = 1000);

	void UploadPerInstanceData(const DirectX::XMMATRIX& mat, const UINT& materialIndex, UINT indexOffset);

	//Vertex Shader Input
	RVertexBuffer VertexBufferPositions;
	RStructuredBuffer VertexBufferRest;
	RIndexBuffer IndexBuffer;

	//Instanced Draw Data
	RStructuredBuffer AllInstancesModelMatricesGPU;
	RByteAddressBuffer AllInstancesIndicesToMaterialsGPU;

};