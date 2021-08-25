#include "Scene.h"

#include "Renderer.h"
#include "Utilities/ModelImporter.h"
#include "DrawableComponent/Drawable.h"

#include "LightComponent/Lights.h"
#include "DrawableComponent/DebugDraw.h"

#include "GpuUploader.h"

RScene::RScene() :
	CameraMain(SRenderer::Get().GetViewportSize())
{
	assert(!pSingleton);
	pSingleton = this;

	GlobalLight.SetPosition({ 0, 1, 0 });

	SceneUniformBuffer.Create(sizeof(RSceneUniformBufferStruct));

	//Create DefaultMaterial
	RMaterialManagerS::CreateNew("Default");

	//Create Main Instances
	DefaultDrawables[EDefaultDrawableType_Sphere].Create("sphere");
	DefaultDrawables[EDefaultDrawableType_Box].Create("box");
	DefaultDrawables[EDefaultDrawableType_Plane].Create("plane");

	RLightSource::InitDrawablesAndMaterial();

	/*CreateLight(LIGHT_TYPE_POINT);
	Lights.back().SetPosition({ -2.5, 0, 5.f });*/

	LoadFromFile();
}

void RScene::OnUpdate()
{
	RScopedProfileEvent SceneUpdate("SceneUpdate");

	CameraMain.OnUpdate();

	RMaterialManagerS::OnUpdate();

	for (auto& light : Lights)
	{
		light.OnUpdate();
	}
	RLightSource::UpdateLightDrawables();

	UpdateUniformBuffer();

	for (auto& drawable : DefaultDrawables)
	{
		drawable.OnUpdate();
	}

	RGpuUploaderS::OnUpdate(*this);
}

void RScene::RegisterSceneObject(RLevelEditorObject* object)
{
	static int c = 0;
	std::string NameCopy = object->Name;
	while (SceneObjectNames.find(NameCopy) != SceneObjectNames.end())
	{
		NameCopy = object->Name + std::to_string(c);
		c++;
	}
	object->Name = NameCopy;
	SceneObjectNames.insert(object->Name);
}

void RScene::UnregisterSceneObject(RLevelEditorObject* object)
{
	SceneObjectNames.erase(object->Name);
}

RScene* RScene::pSingleton = nullptr;
