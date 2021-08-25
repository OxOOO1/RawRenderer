#pragma once

#include <string>
#include <Windows.h>

class RLevelEditorObject
{
public:


	RLevelEditorObject() = default;

	RLevelEditorObject(const std::string& name)
		:Name(name)
	{

	}

	virtual void RenderUI() = 0;

	virtual void RenderUIGlobal();

	virtual void Selectable(RLevelEditorObject*& pObject);

	std::string Name;

	bool bDrawInSeparateWindow = false;

	bool bAttachedToCam = false;
};