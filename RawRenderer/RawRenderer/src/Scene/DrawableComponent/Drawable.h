#pragma once

#include "Geometry.h"
#include "Material.h"
#include "VertexComponent.h"
#include "PipelineState.h"
#include "Scene/Transform/Transform.h"
#include "SceneObject.h"
#include "Instancing.h"
#include "DrawingState.h"
#include "DrawableGpuData.h"

using namespace DirectX;

class RLevelEditorObject;
class RScene;

//Single DrawCall data
class RDrawableMesh
{
public:

	RDrawableMesh() = default;

	RDrawableMesh(const std::string& name);

	void Create(const std::string& name);

	void CreateGpuResources()
	{
		GpuData.CreateVertexIndexBuffers(GeometryBufferCPU);

		GpuData.CreateInstanceBuffers(Name);
	}
	void DestroyGpuResources()
	{

	}

	virtual void OnUpdate()
	{
		RInstancesManagerS::OnUpdate(*this);
	}

	UINT CreateNewInstance();

	UINT GetInstanceCount()
	{
		return DrawableInstances.size();
	}

	RDrawableInstance& GetMainInstance()
	{
		return DrawableInstances.front();
	}

	RDrawableInstance& GetInstance(UINT index)
	{
		return DrawableInstances.at(index);
	}

	RDrawableInstance& GetLastInstance()
	{
		return DrawableInstances.back();
	}

public:
	//Shared between Instances
	RGeometryBufferCPU GeometryBufferCPU;
	RDrawDataGPU GpuData;

	std::vector<RDrawableInstance> DrawableInstances;

	std::string Name;

};


