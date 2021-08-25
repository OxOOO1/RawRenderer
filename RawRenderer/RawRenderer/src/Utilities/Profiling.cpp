#include "Profiling.h"

#include "Types.h"

#include "thirdParty/ImGUI/imgui.h"

//
//std::vector<RProfileEventNode> RProfileEvent::EventNodes;
//
//int RProfileEvent::IndexToCurrentEventNode = -1;
std::vector<RProfileEventNode> EventNodes;
std::vector<RProfileEventNode> PrevFrameEventNodes;

int IndexToCurrentEventNode{ -1 };

void RProfileEvent::BeginNew(const std::string& title, RCommandList* inCmdList /*= nullptr*/)
{
	UINT ThisNewNodeIndex = EventNodes.size();

	if (IndexToCurrentEventNode != -1)
	{
		auto& ParentNode = EventNodes.at(IndexToCurrentEventNode);
		ParentNode.AddChildNode(ThisNewNodeIndex);
	}

	EventNodes.emplace_back(title, IndexToCurrentEventNode, inCmdList);

	IndexToCurrentEventNode = ThisNewNodeIndex;

	if (inCmdList)
		inCmdList->PIXBeginEvent(std::wstring{ title.begin(), title.end() }.c_str());
	else
		::PIXBeginEvent(0, title.c_str());
}

void RProfileEvent::EndCurrent()
{
	auto& EventNode = EventNodes.at(IndexToCurrentEventNode);
	EventNode.CalculateProfileResult();

	IndexToCurrentEventNode = EventNode.IndexToParent;

	if (EventNode.CmdList)
		EventNode.CmdList->PIXEndEvent();
	else
		::PIXEndEvent();
}

void RProfileEvent::ShowProfileUI()
{
	if (ImGui::Begin("Profile"))
	{
		for (auto& node : PrevFrameEventNodes)
		{
			std::string s = node.Name + " " + std::to_string(node.ProfileResultMs) + "ms";
			ImGui::Text(s.c_str());
		}
	}
	ImGui::End();
}

void RProfileEvent::EndFrame()
{
	PrevFrameEventNodes = std::move(EventNodes);
	assert(EventNodes.empty());
}

