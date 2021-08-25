#pragma once

#include "LLRenderer.h"
#include "Passes/BasePass.h"
#include "Passes/BeginPresentPass.h"
#include "Resources/CommonResources.h"
#include "Scene/LevelEditor.h"

#include "Utilities/Profiling.h"

class SRenderer
{

public:

	static SRenderer& Get()
	{
		return *pSingleton;
	}

	SRenderer(UINT2 RTSize)
		: CommonResources(RTSize),
		RenderTargetSize(RTSize), ViewportSize(RTSize)
	{
		assert(pSingleton == nullptr);
		pSingleton = this;

		Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, (FLOAT)ViewportSize.x, (FLOAT)ViewportSize.y };
		ScissorRect = CD3DX12_RECT{ 0, 0, (LONG)RenderTargetSize.x, (LONG)RenderTargetSize.y };


		BasePass.InitResources();
		BeginPresentPass.InitResources();


		Scene = std::make_unique<RScene>();


	}

	void PrepareMainRenderTargets()
	{
		RCommandListGraphics& CmdList = RCommandList::BeginNew(L"ClearMainRenderTargets").GetGraphicsContext();

		//Set global states 
		auto& ResourceManager = RCommonGpuResourcesS::Get();

		auto& RT = ResourceManager.SceneColor;
		auto& DSV = ResourceManager.SceneDepth;

		CmdList.TransitionResource(RT, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		CmdList.TransitionResource(DSV, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

		CmdList.ClearRenderTarget(RT);
		CmdList.ClearDepth(DSV);

		CmdList.ExecuteCmdListAndReleaseContext();
	}

	void Render(SLLRenderer& LLRenderer)
	{
		RScopedProfileEvent RenderMainEvent("Render");
		//Update
		Scene->OnUpdate();

		//BeginRendering
		PrepareMainRenderTargets();

		//DepthPass

		//BasePass
		RBasePass::Input BasePassInput;
		BasePassInput.DepthTarget = &RCommonGpuResourcesS::Get().SceneDepth;
		BasePassInput.RenderTargetColor = &RCommonGpuResourcesS::Get().SceneColor;
		BasePassInput.scene = Scene.get();
		RBasePass::Output BasePassOutput = BasePass.Execute(BasePassInput);

		RLevelEditor::Render();

		RBeginPresentPass::Input BeginPresentPassInput;
		BeginPresentPassInput.ColorRTMain = BasePassOutput.RenderTargetColor;
		RBeginPresentPass::Output BeginPresentPassOutput = BeginPresentPass.Execute(BeginPresentPassInput);
		LLRenderer.Present();

	}

	void SetRenderTargetSize(UINT2 size)
	{
		RenderTargetSize = size;
		if (RenderTargetSize < ViewportSize)
			SetViewportSize(RenderTargetSize);
	}

	void SetViewportSize(UINT2 size)
	{
		ViewportSize = size;
	}

	UINT2 GetViewportSize()
	{
		return ViewportSize;
	}

	UINT2 GetRTSize()
	{
		return RenderTargetSize;
	}

	const CD3DX12_VIEWPORT& GetViewport()
	{
		return Viewport;
	}

	const CD3DX12_RECT& GetScissorRect()
	{
		return ScissorRect;
	}

private: //Scene

	std::unique_ptr<RScene> Scene;


private: //Resources

	RCommonGpuResourcesS CommonResources;

//Info
	UINT2 RenderTargetSize;
	UINT2 ViewportSize;

	CD3DX12_VIEWPORT Viewport;
	CD3DX12_RECT ScissorRect;

private: //Passes

	RBasePass BasePass;
	RBeginPresentPass BeginPresentPass;


private:
	static SRenderer* pSingleton;

};