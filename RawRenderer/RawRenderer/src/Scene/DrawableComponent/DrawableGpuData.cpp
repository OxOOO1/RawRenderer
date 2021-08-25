#include "DrawableGpuData.h"

#include "DrawableComponent/Geometry.h"
#include "CommandContext.h"

void RDrawDataGPU::CreateVertexIndexBuffers(RGeometryBufferCPU& mesh)
{
	VertexBufferPositions.Create(L"VertexBuffer", mesh.VertexBufferPositionsCPU);
	VertexBufferRest.Create(L"VertexBufferRest", mesh.VertexBufferRestCPU.GetNumVertices(), mesh.VertexBufferRestCPU.GetVertexSize(), mesh.VertexBufferRestCPU.Data.data());
	IndexBuffer.Create(L"IndexBuffer", mesh.IndexBufferCPU.Indices.size(), sizeof(unsigned int), mesh.IndexBufferCPU.Indices.data());
}

void RDrawDataGPU::CreateInstanceBuffers(const std::string& name, UINT NumInstances /*= 1000*/)
{
	auto BufferName = std::wstring{ name.begin(), name.end() } + L"_AllInstancesModelMatrices";
	AllInstancesModelMatricesGPU.Create(BufferName, NumInstances, sizeof(XMMATRIX));

	BufferName = std::wstring{ name.begin(), name.end() } + L"_AllInstancesIndicesToMaterials";
	AllInstancesIndicesToMaterialsGPU.Create(BufferName, NumInstances, sizeof(UINT32));
}

void RDrawDataGPU::UploadPerInstanceData(const DirectX::XMMATRIX& mat, const UINT& materialIndex, UINT indexOffset)
{
	RCommandList::UploadToBufferImmediate(AllInstancesModelMatricesGPU, &mat, sizeof(XMMATRIX), sizeof(XMMATRIX) * indexOffset);
	RCommandList::UploadToBufferImmediate(AllInstancesIndicesToMaterialsGPU, &materialIndex, sizeof(UINT32), sizeof(UINT32) * indexOffset);
}
