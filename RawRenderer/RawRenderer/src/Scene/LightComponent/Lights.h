#pragma once

#include "Scene/ImGuiObject.h"
#include "Scene/View.h"
#include "Utilities/Types.h"
#include "Resources/GpuBuffer.h"

#include "SceneObject.h"
#include "shaders/LightDesc.h"

using namespace DirectX;

class RGlobalLight : public RSceneView, public RLevelEditorObject 
{
public:

	RGlobalLight()
		: RLevelEditorObject("GLobalLight")
	{}

	virtual void RenderUI() override;

	float3 RotationPitchYawRoll{ 0,0,0 };

	void SetPosition(RFloat3 position)
	{
		position.Normalize();
		Position = position;
		Direction = -position;
	}

	struct Desc
	{
		float3 DirectionViewSpace{ 0, -1, 0 };
		float Intensity;
		float3 Color;
		float AmbientLightIntensity;
	};
	static_assert(sizeof(Desc) % 16 == 0);
};


class RLight
{
public:

	RLight(uint LightType = LIGHT_TYPE_POINT);

	void OnUpdate();

public:
	bool bAttachedToCam = false;
	bool bNeedsGpuUpload = false;
	uint Index = 0;

	LightDesc Desc;

};

class RDrawableMesh;
class RDrawableInstance;
class RMaterial;

class RLightSource : public RLevelEditorObject
{
public:

	RLightSource(uint LightType = LIGHT_TYPE_POINT);

	virtual void RenderUI() override;

	RDrawableInstance& GetDrawableInstance();

	RMaterial& GetMaterial();

	void OnUpdate();

	void SetPosition(const float3& position);
	void SetRadius(float radius)
	{
		Light.Desc.Radius = radius;
		Light.bNeedsGpuUpload = true;
	}

public:
	RLight Light;

	UINT IndexToDrawable;

public:
	static void InitDrawablesAndMaterial();

	static void UpdateLightDrawables();

	static RDrawableMesh PointLightDrawable;
	static RDrawableMesh SpotLightDrawables;

	static UINT IndexToMainMaterial;

};