#pragma once

#include <vector>
#include <string>

#include "Resources/GpuBuffer.h"
//Don't include Transform.h

//#include "Scene/Scene.h"


/*
template <typename T>
class RNode
{
public:

	RNode() = default;

	UINT Index;
	int IndexToParent = -1;
	std::vector<UINT> IndicesToChildren;

	T Data;
};

template <typename T>
class RNodeTree
{
public:
	std::vector<RNode<T>> Nodes;
};*/

/*
class RMeshViewNode : public RNode
{
public:
	RMeshDesc MeshDesc;
};*/


/*
//Holds View to a SubMesh in Mesh, can have multiple instances for InstancedDraw
class RDrawableInstancedNode
{
public:

	RDrawableInstancedNode()
	{
		Instances.emplace_back();
	}

	void AddChild(UINT index)
	{
		IndicesToChildren.push_back(index);
	}

	RMeshDesc GetMeshDesc()
	{
		return MeshDesc;
	}

public:
	std::vector<RDrawableInstance> Instances;

	RMeshDesc MeshDesc;

	std::string Name;

	UINT Index = 0;
	int IndexToParent = -1;
	std::vector<UINT> IndicesToChildren;

};

class RDrawableInstancedNodesTree : public RDrawableUsedComponent
{
public:

	virtual void Init(RDrawable& owner) override
	{
		UpdateAllNodesTransforms();
		UploadTransformDataToGpu();
	}

	virtual void OnUpdate(RDrawable& owner) override;

	int RenderUI();

	RDrawableInstancedNode& GetRootNode()
	{
		assert(!Nodes.empty());
		return Nodes.at(0);
	}

private:
	void CreateTransformsBufferGPU();

	void UpdateAllNodesTransforms(UINT nodeIndex = 0, const DirectX::XMMATRIX& parentTransform = DirectX::XMMatrixIdentity());

	void UploadTransformDataToGpu()
	{
		for (auto& node : Nodes)
		{
			if (node.bNeedsUpdate)
			{
				auto UploadMat = DirectX::XMMatrixTranspose(node.Transform.MatWorldSpace);
				TransformCBufferPool.UpdateCBuffer(node.TransformCBufferRef, &UploadMat);

				node.bNeedsUpdate = false;

			}
		}
	}


public:
	std::vector<RDrawableInstancedNode> Nodes;

	bool bAttachedToCam = false;

};*/