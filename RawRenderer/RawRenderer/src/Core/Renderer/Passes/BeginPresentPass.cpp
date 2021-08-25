#include "BeginPresentPass.h"

#include "LLRenderer.h"
#include "Renderer.h"

#include "Utilities/ImGuiManager.h"

RBeginPresentPass::Output RBeginPresentPass::Execute(Input& input)
{
	RCommandListGraphics& CmdList = RCommandListGraphics::BeginNew(L"Present");

	auto& RTSRV = *input.ColorRTMain;

	CmdList.TransitionResource(RTSRV, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	CmdList.SetRootSignature(RootSignature);

	CmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CmdList.SetDynamicDescriptor(0, 0, RTSRV.GetSRV());

	RColorBuffer& DestRT = SLLRenderer::GetCurFramebuffer();

	auto& Renderer = SRenderer::Get();
	auto& LLRenderer = SLLRenderer::Get();
	uint2 OutputSize = LLRenderer.GetOutputSize();
	uint2 RTSize = Renderer.GetRTSize();
	uint2 ViewportSize = Renderer.GetViewportSize();

	/*if (ViewportSize == OutputSize)
	{
		CmdList.SetPipelineState(PSODefault);
		CmdList.TransitionResource(DestRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		CmdList.SetRenderTarget(DestRT.GetRTV());
		CmdList.SetViewportAndScissor(0,0, OutputSize.x, OutputSize.y);
		CmdList.Draw(3);
	}
	else
	{
		assert(ViewportSize < OutputSize);

		if (UpscaleFilter == EUpscaleFilterType::Bilinear)
		{*/
			CmdList.SetPipelineState(PSODefault);
			CmdList.TransitionResource(DestRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			CmdList.SetRenderTarget(DestRT.GetRTV());
			CmdList.SetViewportAndScissor(0, 0, OutputSize.x, OutputSize.y);
			CmdList.Draw(3);
	/*	}
	}*/

	//Render UI
	CmdList.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, SLLRenderer::GetGPUViewsDescHeap().Get());
	ImGui::Render();
	SImGuiManager::SubmitToCmdList(CmdList.GetCommandList());

	CmdList.TransitionResource(DestRT, D3D12_RESOURCE_STATE_PRESENT);

	CmdList.ExecuteCmdListAndReleaseContext();

	Output output;
	output.CurrentSwapchainBackBuffer = &DestRT;
	return output;
}

void RBeginPresentPass::InitResources()
{
	

	PSODefault.SetRootSignature(RootSignature);
	PSODefault.SetRasterizerState(RCommonGpuResourcesS::Get().RasterizerTwoSided);
	//PSODefault.SetBlendState(SCommonResources::Get().BlendPreMultiplied);
	PSODefault.SetBlendState(RCommonGpuResourcesS::Get().BlendDisable);
	PSODefault.SetDepthStencilState(RCommonGpuResourcesS::Get().DepthStateDisabled);
	PSODefault.SetSampleMask(0xFFFFFFFF);
	PSODefault.SetInputLayout(0, nullptr);
	PSODefault.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	PSODefault.SetVertexShader(VertexShader.Get());
	PSODefault.SetPixelShader(PixelShader.Get());
	PSODefault.SetRenderTargetFormat(SLLRenderer::SWAPCHAIN_FORMAT, DXGI_FORMAT_UNKNOWN);
	PSODefault.InitGetOrCreate();


}
