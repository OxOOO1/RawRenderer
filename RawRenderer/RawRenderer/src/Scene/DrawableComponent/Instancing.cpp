#include "Instancing.h"

#include "thirdParty/ImGUI/imgui.h"
#include "Scene.h"
#include "BoundingBoxes/BoundingVolume.h"
#include "Scene/DrawableComponent/Drawable.h"

void RDrawableInstance::RenderUI()
{
	ImGui::BeginGroup();
	ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us

	ImGui::Separator();

	if (ImGui::Button("Go To"))
	{
		RBoundingBox& bbox = RBoundingVolumesManagerS::GetBox(IndexToBBox);
		RFloat3 BboxExtents = bbox.GetExtents();
		RScene::Get().GetCamera().GoTo(Transform.Position, BboxExtents.GetLength() * 1.5f);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("AttachToCam", &bAttachedToCam))
	{
	}
	/*if (ImGui::SliderAngle("Rotation Pitch", &Geometry.NodesManager.GetRootNode().Transform.Rotation.y, -180.f, 180.f))
	{

	}
	if (ImGui::SliderAngle("Rotation Yaw", &Geometry.NodesManager.GetRootNode().Transform.Rotation.x, -180.f, 180.f))
	{

	}*/
	if (ImGui::Button("Rotate90Y"))
	{
		Transform.RotateY90();
	}
	if (ImGui::Button("Rotate90X"))
	{
		Transform.RotateX90();
	}

	ImGui::Separator();

	ImGui::BeginChild("rootnode", ImVec2(0, 350), true); // Leave room for 1 line below us
	ImGui::BeginGroup();

	//Main
	int3 Position = Transform.Position;
	if (ImGui::DragInt3("Position", &Position.x)) {

		Transform.SetPosition(Position);
	}
	
	float scale = Transform.Scale.x;
	if (ImGui::SliderFloat("Scale", &scale, 0.1f, 10.f))
	{
		Transform.SetUniformScale(scale);
	}

	//Material
	ImGui::Dummy(ImVec2(10, 10));
	ImGui::Spacing();
	ImGui::NewLine();
	ImGui::Separator();

	auto& material = RMaterialManagerS::GetMaterial(IndexToMaterial);

	material.RenderUI();

	static char NewMaterialName[25];
	if (ImGui::InputText("MaterialName", NewMaterialName, 25))
	{

	}
	if (ImGui::Button("CreateNewMaterial"))
	{
		std::string Name(NewMaterialName, 25);
		IndexToMaterial = RMaterialManagerS::CreateNew(Name);
		std::memset(NewMaterialName, '\0', 25);
		bWasUpdated = true;
	}

	ImGui::EndGroup();
	ImGui::EndChild();

	ImGui::Text("AllMaterials");

	/*int MatCounter = 0;
	for (auto& mat : Materials)
	{
		if (ImGui::ColorButton(std::string{ mat.Name + std::to_string(MatCounter) }.c_str(), ImVec4{ mat.Diffuse.x, mat.Diffuse.y, mat.Diffuse.z, 1.f }))
		{
			SelectedMaterial = mat.Index;
		}
	}*/

	ImGui::EndChild();
	ImGui::EndGroup();
}

UINT RInstancesManagerS::CreateNewInstance(RDrawableMesh& owner)
{
	auto index = owner.DrawableInstances.size();
	owner.DrawableInstances.emplace_back(owner.Name + std::to_string(index));
	//owner.DrawableInstances.back().IndexToMaterial = indexToMaterial;
	return index;
}

void RInstancesManagerS::OnUpdate(RDrawableMesh& owner)
{
	for (int i = 0; i < owner.DrawableInstances.size(); i++)
	{
		auto& instance = owner.DrawableInstances[i];
		if (instance.bAttachedToCam)
		{
			auto& cam = RScene::Get().GetCamera();
			auto& bbox = RBoundingVolumesManagerS::GetBox(instance.IndexToBBox);
			instance.Transform.SetPosition(cam.GetPosition() + (cam.GetDirection() * RFloat3(bbox.GetExtents()).GetLength() * 1.5f));
		}
		if (instance.bWasUpdated || instance.Transform.bWasUpdated)
		{
			auto& bbox = RBoundingVolumesManagerS::GetBox(instance.IndexToBBox);
			bbox.SetPosition(instance.Transform.Position);

			XMMATRIX mat = DirectX::XMMatrixTranspose(instance.Transform.GetTransformMatrix());

			owner.GpuData.UploadPerInstanceData(mat, instance.IndexToMaterial, i);

			instance.bWasUpdated = false;
			instance.Transform.bWasUpdated = false;

		}

	}
}
