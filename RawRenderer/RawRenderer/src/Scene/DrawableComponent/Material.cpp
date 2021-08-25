#include "Material.h"

#include <string>
#include <array>

#include "thirdParty/ImGUI/imgui.h"
#include "Renderer/CommandContext.h"
#include "Resources/CommonResources.h"

void RMaterial::RenderUI()
{
	ImGui::Text(Name.c_str());

	if (ImGui::ColorEdit3("Albedo", &Desc.Color.x))
		bNeedsGpuUpload = true;

	if (ImGui::SliderFloat("Specular Power", &Desc.SpecularPower, 0.1f, 512.f))
		bNeedsGpuUpload = true;

	bool bEmmiter = Desc.Type & MAT_TYPE_EMITTER;
	if (ImGui::Checkbox("Emitter", &bEmmiter))
	{
		if (bEmmiter)
		{
			Desc.Type |= MAT_TYPE_EMITTER;
		}
		else
		{
			Desc.Type = Desc.Type & ~MAT_TYPE_EMITTER;
		}
		bNeedsGpuUpload = true;
	}

	if (ImGui::SliderFloat("Brightness", &Desc.Color.w, 0.1f, 10.f))
		bNeedsGpuUpload = true;
		

	std::array<std::string, 7> matIDs;

	for (int i = 0; i < 7; i++)
	{
		matIDs[i] = std::to_string(i);
	}

	static int item_current_idx = Desc.TextureId;                // Here our selection data is an index.
	const char* combo_label = matIDs[item_current_idx].c_str();  // Label to preview before opening the combo (technically could be anything)(
	if (ImGui::BeginCombo("TextureId", combo_label, 0))
	{
		for (int n = 0; n < matIDs.size(); n++)
		{
			const bool is_selected = (item_current_idx == n);
			if (ImGui::Selectable(matIDs[n].c_str(), is_selected))
			{
				Desc.TextureId = item_current_idx = n;
				bNeedsGpuUpload = true;
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

}

UINT RMaterialManagerS::CreateNew(const std::string& name, UINT type /*= RMaterial::Default*/)
{
	auto& Manager = Get();
	//Index & Name
	auto Index = Manager.Materials.size();
	assert(Index < MAX_NUM_MATERIALS - 1);

	static int c = 0;
	std::string NameCopy = name;
	while (Manager.MaterialNameIndexMap.find(NameCopy) != Manager.MaterialNameIndexMap.end())
	{
		NameCopy = name + std::to_string(c);
		c++;
	}
	Manager.MaterialNameIndexMap[NameCopy] = Index;

	Manager.Materials.emplace_back(NameCopy, type);

	return Index;
}

void RMaterialManagerS::OnUpdate()
{
	
}

const D3D12_CPU_DESCRIPTOR_HANDLE& RMaterialManagerS::GetSRV()
{
	auto& descBuffer = RCommonGpuResourcesS::Get().AllMaterialsDescriptorsBuffer;
	return descBuffer.GetSRV();
}

RMaterialManagerS::RMaterialManagerS()
{
	auto& descBuffer = RCommonGpuResourcesS::Get().AllMaterialsDescriptorsBuffer;
	descBuffer.Create(L"AllMaterialDescriptors", MAX_NUM_MATERIALS, sizeof(MaterialDesc));
}
