#pragma once

class SImGuiManager
{
public:
	SImGuiManager(void* hWnd);
	~SImGuiManager();

	void NewFrame();
	static void SubmitToCmdList(struct ID3D12GraphicsCommandList* CmdList);

	bool pOpen = true;
};