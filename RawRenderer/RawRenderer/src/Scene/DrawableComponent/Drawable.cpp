#include "Drawable.h"

#include "Utilities/ModelImporter.h"
#include "Scene.h"
#include "BoundingBoxes/BoundingVolume.h"


RDrawableMesh::RDrawableMesh(const std::string& name)
{
	Create(name);
}

void RDrawableMesh::Create(const std::string& name)
{
	Name = name;

	auto fileName = std::string{ "assets/models/" + name + ".fbx" };
	RModelImporter::Import(fileName, GeometryBufferCPU);

	CreateGpuResources();
}

UINT RDrawableMesh::CreateNewInstance()
{
	UINT index = RInstancesManagerS::CreateNewInstance(*this);
	UINT bboxIndex = RBoundingVolumesManagerS::CreateNewBox(GeometryBufferCPU);
	GetLastInstance().IndexToBBox = bboxIndex;
	return index;
}
