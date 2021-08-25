#pragma once

#include <vector>
#include "LightComponent/Lights.h"

class RGpuUploaderS
{
public:

	static void UploadLights(std::vector<RLightSource>& lights);

	static void UploadMaterials();

	static void OnUpdate(class RScene& scene);
};