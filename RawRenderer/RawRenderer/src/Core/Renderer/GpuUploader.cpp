#include "GpuUploader.h"

#include "CommandContext.h"
#include "Scene/Scene.h"

void RGpuUploaderS::UploadLights(std::vector<RLightSource>& lights)
{
	for (int i = 0; i < lights.size(); i++)
	{
		auto& light = lights[i].Light;
		if (light.bNeedsGpuUpload)
		{
			auto LightDesc = light.Desc;
			RCommandList::UploadToBufferImmediate(RCommonGpuResourcesS::Get().LightsBuffer, &LightDesc, sizeof(RLight::Desc), i * sizeof(RLight::Desc));
			light.bNeedsGpuUpload = false;
		}
	}
}

void RGpuUploaderS::UploadMaterials()
{
	auto& manager = RMaterialManagerS::Get();
	for (int i = 0; i < manager.Materials.size(); i++)
	{
		auto& mat = manager.Materials[i];

		if (mat.bNeedsGpuUpload)
		{
			auto& descBuffer = RCommonGpuResourcesS::Get().AllMaterialsDescriptorsBuffer;
			RCommandList::UploadToBufferImmediate(descBuffer, &mat.Desc, sizeof(MaterialDesc), sizeof(MaterialDesc) * i);
			mat.bNeedsGpuUpload = false;
		}
	}
}

void RGpuUploaderS::OnUpdate(class RScene& scene)
{
	UploadLights(scene.Lights);
	UploadMaterials();
}
