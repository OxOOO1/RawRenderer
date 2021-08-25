#include "ImGuiManager.h"

#include "LLRenderer.h"

#include "thirdParty/ImGUI/imgui.h"
#include "thirdParty/ImGUI/imgui_impl_dx12.h"
#include "thirdParty/ImGUI/imgui_impl_win32.h"

SImGuiManager::SImGuiManager(void* hWnd)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);

	auto& DescHeap = SLLRenderer::GetGPUViewsDescHeap();

	RDescriptorHandle DescHandle = DescHeap.Allocate();
	ImGui_ImplDX12_Init(SLLRenderer::GetDevice().Get(), SLLRenderer::SWAPCHAIN_FRAME_COUNT,
		SLLRenderer::SWAPCHAIN_FORMAT, DescHeap.Get(),
		DescHandle.CPU,
		DescHandle.GPU);
}

SImGuiManager::~SImGuiManager()
{
	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void SImGuiManager::NewFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	ImGui::ShowDemoWindow(&pOpen);

	// Rendering
	
}

void SImGuiManager::SubmitToCmdList(ID3D12GraphicsCommandList* CmdList)
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CmdList);
}
