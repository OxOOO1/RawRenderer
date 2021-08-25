#pragma once

#include "SceneObject.h"
#include "Utilities\Types.h"
#include "Transform\Transform.h"

static int SelectedMaterial = -1;
class RDrawableMesh;
//Drawable with its own Transform and Material and MeshLOD
class RDrawableInstance : public RLevelEditorObject
{
public:
	RDrawableInstance(const std::string& name)
		: RLevelEditorObject(name)
	{}

	virtual void RenderUI() override;

	void SetMaterial(UINT index)
	{
		IndexToMaterial = index;
		bWasUpdated = true;
	}

	//RTransform Transform;
	RTransformSimple Transform;

	uint IndexToDrawData = 0;
	uint IndexToMaterial = 0;
	uint IndexToBBox = 0;
	uint CurrentLODLevel = 0;

	bool bWasUpdated = true;

	bool bIsOccluder = false;

};

class RInstancesManagerS
{
public:

	static UINT CreateNewInstance(RDrawableMesh& owner);

	static void OnUpdate(RDrawableMesh& owner);

};

