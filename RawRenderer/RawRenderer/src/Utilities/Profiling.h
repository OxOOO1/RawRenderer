#pragma once

#include "Time.h"
#include <queue>


class RProfileEventNode
{
public:

	RProfileEventNode(const std::string& name, UINT indexToParent, RCommandList* inCmdList = nullptr)
		: Name(name), IndexToParent(indexToParent), StartTime(std::chrono::high_resolution_clock::now()), CmdList(inCmdList)
	{}

	void AddChildNode(UINT index)
	{
		IndicesToChildren.push_back(index);
	}

	void CalculateProfileResult()
	{
		ProfileResultMs = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - StartTime).count() * 1000.f;
	}

	std::vector<UINT> IndicesToChildren;
	std::string Name;
	UINT IndexToParent;

	RCommandList* CmdList;

	std::chrono::time_point<std::chrono::steady_clock> StartTime;
	float ProfileResultMs;
};

namespace RProfileEvent
{
	
	void BeginNew(const std::string& title, RCommandList* inCmdList = nullptr);

	void EndCurrent();

	void ShowProfileUI();

	void EndFrame();
}

class RScopedProfileEvent
{
public:

	RScopedProfileEvent(const std::string& title, RCommandList* inCmdList = nullptr)
	{
		RProfileEvent::BeginNew(title, inCmdList);
	}
	~RScopedProfileEvent()
	{
		RProfileEvent::EndCurrent();
	}

};