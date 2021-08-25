#pragma once

class IImGuiObject
{
public:

	virtual void RenderImGuiWindow() = 0;

	virtual void ResetImGuiWindow()
	{

	}

	bool bImGuiWindowEnabled = false;
};