#include "Node.h"

#include <optional>

#include "Scene/Scene.h"


/*
void RenderNodeUITree(RDrawableInstancedNodesTree& nodesManager, UINT indexToNode, std::optional<UINT>& selectedIndex)
{
	auto& node = nodesManager.Nodes.at(indexToNode);

	const auto node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
		((node.Index == selectedIndex) ? ImGuiTreeNodeFlags_Selected : 0)
		| ((node.IndicesToChildren.empty()) ? ImGuiTreeNodeFlags_Leaf : 0)
		;

	const auto expanded = ImGui::TreeNodeEx((void*)(intptr_t)node.Index, node_flags, node.Name.c_str());
	if (ImGui::IsItemClicked())
	{
		selectedIndex = node.Index;
	}
	if (expanded)
	{
		for (const auto i : node.IndicesToChildren)
		{
			RenderNodeUITree(nodesManager, i, selectedIndex);
		}
		ImGui::TreePop();
	}
}*/

/*
void RDrawableInstancedNodesTree::CreateTransformsBufferGPU()
{
	auto& Name = GetRootNode().Name;
	TransformCBufferPool.Create(std::wstring{ Name.begin(), Name.end() }, Nodes.size() * sizeof(DirectX::XMMATRIX));

	for (auto& node : Nodes)
	{
		node.TransformCBufferRef = TransformCBufferPool.CreateCBuffer(sizeof(DirectX::XMMATRIX));
	}
}*/

/*
void RDrawableInstancedNodesTree::OnUpdate(RDrawable& owner)
{
	UpdateAllNodesTransforms();

	UploadTransformDataToGpu();

}*/

/*
void RDrawableInstancedNodesTree::UpdateAllNodesTransforms(UINT nodeIndex, const DirectX::XMMATRIX& parentTransform)
{
	auto& node = Nodes.at(nodeIndex);

	if (node.bNeedsUpdate)
	{
		node.Transform.UpdateTransformLocal();
	}

	auto t = DirectX::XMMatrixMultiply(node.Transform.MatLocalSpace, parentTransform);
	/ *auto t = DirectX::XMMatrixMultiply(node.Transform.MatLocalSpace, node.Transform.MatParentSpace);
	t = DirectX::XMMatrixMultiply(t, parentTransform);* /

	if (node.bNeedsUpdate)
	{
		node.Transform.MatWorldSpace = DirectX::XMMatrixTranspose(t);
	}

	for (const auto i : node.IndicesToChildren)
	{
		UpdateAllNodesTransforms(i, t);
	}
}*/

/*
std::optional<UINT> selectedIndex = std::nullopt;

bool bShowSpaces = false;

int RDrawableInstancedNodesTree::RenderUI()
{
	ImGui::Columns(2, nullptr, true);

	RenderNodeUITree(*this, 0, selectedIndex);

	ImGui::NextColumn();

	if (selectedIndex)
	{
		auto& node = Nodes.at(selectedIndex.value());
		auto nodeID = node.Index;

		int3 Position = node.Transform.Position;
		if (ImGui::DragInt3("Position", &Position.x)) {
			node.SetPosition(Position);
		}

		ImGui::Checkbox("ShowViewSpace", &bShowSpaces);
		
		if (bShowSpaces)
		{
			auto& Camera = RScene::Get().GetCamera();
			float3 posView = node.Transform.Position.Transform(Camera.Matrices.View);
			ImGui::DragFloat3("PositionView", &posView.x);
			float4 posProj = posView;
			posProj = RFloat4{ posProj }.Transform(Camera.Matrices.Projection);
			//ImGui::DragFloat4("Position ProjSpace", &posProj.x);
			float2 CamNearFarPlanes = Camera.GetNearFarPlanes();

			//float ClipSpaceZ = posView.z / (CamNearFarPlanes.y - CamNearFarPlanes.x);
			float ClipSpaceZLinear = (posView.z - CamNearFarPlanes.x) / (CamNearFarPlanes.y - CamNearFarPlanes.x);
			float ClipSpaceZRaw = (1.f / posView.z - 1.f / CamNearFarPlanes.x) / (1.f / CamNearFarPlanes.y - 1.f / CamNearFarPlanes.x);

			float3 homogenousSpace = { posProj.x / posProj.w, posProj.y / posProj.w };
			ImGui::DragFloat2("PositionClip", &homogenousSpace.x);
			ImGui::DragFloat("LinearZ", &ClipSpaceZLinear);
			ImGui::DragFloat("DepthBufferZ", &ClipSpaceZRaw, 0.1f, 0.f, 1.f, "%.10f");
		}
		
		if (!node.IndicesToMeshes.empty())
		{
			return node.IndicesToMeshes.back();
		}
		
	}
	return -1;
}*/
