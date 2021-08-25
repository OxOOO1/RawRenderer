#pragma once

#include "Utilities/Types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "Resources/GpuBuffer.h"
#include "shaders/MaterialDesc.h"
static_assert((sizeof(MaterialDesc) % 16) == 0);

class RMaterial
{
	friend class RMaterialManagerS;

public:
	RMaterial(const std::string& name, UINT MatType)
		: Name(name)
	{
		Desc.Color = float4(0.5, 0.5, 0.5, 1);
		Desc.Type = MatType;
		Desc.SpecularPower = 64.f;
		Desc.Opacity = 1.f;
		Desc.TextureId = 0;
	}

	void SetColor(const float3& color)
	{
		Desc.Color = float4(color);
		bNeedsGpuUpload = true;
	}

	void RenderUI();

	MaterialDesc Desc;
	std::string Name;

	bool bNeedsGpuUpload = true;

};

class RMaterialManagerS
{
	friend class RGpuUploaderS;
public:

	static constexpr int MAX_NUM_MATERIALS = 500;

	static UINT CreateNew(const std::string& name, UINT MatType = MAT_TYPE_DEFAULT);

	static RMaterial& GetMaterial(UINT index)
	{
		return Get().Materials.at(index);
	}

	static void OnUpdate();

	static const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV();

private:

	RMaterialManagerS();

	static RMaterialManagerS& Get()
	{
		static RMaterialManagerS MaterialManager;
		return MaterialManager;
	}

	std::unordered_map<std::string, UINT> MaterialNameIndexMap;
	std::vector<RMaterial> Materials;

};