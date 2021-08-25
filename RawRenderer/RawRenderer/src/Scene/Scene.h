#pragma once

#include "Camera.h"
#include <unordered_map>
#include <queue>
#include <vector>
#include <array>
#include <set>

#include "DrawableComponent/Drawable.h"

#include "UniformBuffer.h"

#include "Utilities/Profiling.h"

class RLight;
class RLevelEditorObject;

//Stores and registers all drawables
class RScene
{
public:

	RScene();

	~RScene()
	{
		SaveToFile();
	}

	static RScene& Get()
	{
		assert(pSingleton);
		return *pSingleton;
	}

	void OnEvent(Event& event)
	{
		CameraMain.OnEvent(event);
	}
	void OnUpdate();

	RCamera& GetCamera()
	{
		return CameraMain;
	}

//ObjectCreation
	static void CreateModel(const std::string& name)
	{
		/*Get().DrawablesNodeBased.emplace_back(name);
		Get().RegisterSceneObject(&Get().DrawablesNodeBased.back());*/
	}

	static void CreateLight(uint LightType = LIGHT_TYPE_POINT)
	{
		Get().Lights.emplace_back(LightType);
	}

private:
	void RegisterSceneObject(RLevelEditorObject* object);
	void UnregisterSceneObject(RLevelEditorObject* object);


public:

	//Globals
	RCamera CameraMain;
	RGlobalLight GlobalLight;

	//Default Drawables
	enum
	{
		EDefaultDrawableType_Sphere = 0,
		EDefaultDrawableType_Box,
		EDefaultDrawableType_Plane,
		//...
		EDefaultDrawableType_Count
	};
	std::array<RDrawableMesh, EDefaultDrawableType_Count> DefaultDrawables;

	//Lights
	static constexpr UINT MaxNumLights = 2048;
	std::vector<RLightSource> Lights;

	std::set<std::string> SceneObjectNames;

private:
	void UpdateUniformBuffer()
	{
		const auto& ViewMatrices = CameraMain.Matrices;
		SceneUniformBufferCPU.View = DirectX::XMMatrixTranspose(ViewMatrices.View);
		SceneUniformBufferCPU.Projection = DirectX::XMMatrixTranspose(ViewMatrices.Projection);
		SceneUniformBufferCPU.ViewProjection = DirectX::XMMatrixTranspose(ViewMatrices.ViewProjection);

		SceneUniformBufferCPU.CameraPosition = CameraMain.GetPosition();

		SceneUniformBufferCPU.NumLights = Lights.size();

		SceneUniformBuffer.UploadToGPU(SceneUniformBufferCPU);
	}
	

public:
	struct RSceneUniformBufferStruct
	{
		DirectX::XMMATRIX View{ DirectX::XMMatrixIdentity() };
		DirectX::XMMATRIX Projection{ DirectX::XMMatrixIdentity() };
		DirectX::XMMATRIX ViewProjection{ DirectX::XMMatrixIdentity() };

		RGlobalLight::Desc GlobalLightDesc;

		float3 CameraPosition;

		uint NumLights;

	};
	static_assert(sizeof(RSceneUniformBufferStruct) % 16 == 0);

	RSceneUniformBufferStruct SceneUniformBufferCPU;
	RUniformBuffer SceneUniformBuffer;

private:

	static RScene* pSingleton;





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


private://SaveToFile

	struct RSceneMetadata
	{
		uint NumSphereInstances;
		uint NumCubeInstances;
		uint NumPlaneInstances;
		uint NumLights;
	};
	static constexpr wchar_t FileName[] = L"SceneMeta";

	struct RDrawableInstanceMeta
	{
		float3 position;
		float3 scale;
		float4 orientation;
	};
	struct RLightSourceMeta
	{
		float3 position;
		float radius;
		UINT Type;
	};


	void UploadDrawable(RDrawableMesh& drawable, FileIO::WriteToFile& file)
	{
		for (auto& instance : drawable.DrawableInstances)
		{
			RDrawableInstanceMeta meta;
			meta.position = instance.Transform.Position;
			meta.scale = instance.Transform.Scale;
			meta.orientation = instance.Transform.Orientation;
			file.Write(&meta);
		}
	}

	void UploadLight(RLightSource& light, FileIO::WriteToFile& file)
	{
		RLightSourceMeta meta;
		meta.position = light.Light.Desc.Position;
		meta.radius = light.Light.Desc.Radius;
		meta.Type = light.Light.Desc.Type;
		file.Write(&meta);
	}

	void SaveToFile()
	{
		RSceneMetadata meta;

		meta.NumSphereInstances = DefaultDrawables[EDefaultDrawableType_Sphere].GetInstanceCount();
		meta.NumCubeInstances = DefaultDrawables[EDefaultDrawableType_Box].GetInstanceCount();
		meta.NumPlaneInstances = DefaultDrawables[EDefaultDrawableType_Plane].GetInstanceCount();
		
		meta.NumLights = Lights.size();

		FileIO::WriteToFile file(FileName);

		file.Write(&meta);

		for (auto& drawable : DefaultDrawables)
		{
			UploadDrawable(drawable, file);
		}

		for (auto& light : Lights)
		{
			UploadLight(light, file);
		}

	}

	void LoadDrawable(RDrawableMesh& drawable, uint NumInstances, FileIO::ReadFromFile& file)
	{
		for (int i = 0; i < NumInstances; i++)
		{
			drawable.CreateNewInstance();
			RDrawableInstanceMeta meta;
			file.Read(&meta);
			auto& newInstance = drawable.GetLastInstance();
			newInstance.Transform.SetPosition(meta.position);
			newInstance.Transform.SetOrientation(RQuaternion(meta.orientation));
			newInstance.Transform.SetUniformScale(meta.scale.x);
		}
	}

	void LoadLight(FileIO::ReadFromFile& file)
	{
		RLightSourceMeta meta;
		file.Read(&meta);
		CreateLight(meta.Type);
		Lights.back().SetPosition(meta.position);
		Lights.back().SetRadius(meta.radius);
	}

	void LoadFromFile()
	{
		RSceneMetadata meta;

		FileIO::ReadFromFile file(FileName);

		file.Read(&meta);

		LoadDrawable(DefaultDrawables[EDefaultDrawableType_Sphere], meta.NumSphereInstances, file);
		LoadDrawable(DefaultDrawables[EDefaultDrawableType_Box], meta.NumCubeInstances, file);
		LoadDrawable(DefaultDrawables[EDefaultDrawableType_Plane], meta.NumPlaneInstances, file);

		for (int i = 0; i < meta.NumLights; i++)
		{
			LoadLight(file);
		}

	}

};

