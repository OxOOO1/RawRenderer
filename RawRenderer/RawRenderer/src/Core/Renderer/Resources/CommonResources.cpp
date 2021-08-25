#include "CommonResources.h"

#include "Scene/Scene.h"

RCommonGpuResourcesS::RCommonGpuResourcesS(UINT2 RTSize)
{
	assert(pSingleton == nullptr);
	pSingleton = this;

	InitDescs();

	//Render Targets
	SceneColor.Create(L"ColorRTMain", RTSize.x, RTSize.y, 1, RTFormat);
	SceneDepth.Create(L"DepthBufferMain", RTSize.x, RTSize.y, DepthFormat);


	//Buffers
	LightsBuffer.Create(L"LightsBuffer", RScene::MaxNumLights, sizeof(RLight::Desc));
}

RCommonGpuResourcesS* RCommonGpuResourcesS::pSingleton = nullptr;

