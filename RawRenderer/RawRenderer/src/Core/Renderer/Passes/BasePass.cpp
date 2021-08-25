#include "BasePass.h"

#include "LLRenderer.h"
#include "Renderer.h"
#include "Scene/DrawableComponent/Drawable.h"


void RBasePass::InitResources()
{
	
	//PSO
	//BasePassCommonPSOs = RCommonPSOsManagerS::InitCommonPSOs(RootSignature, PixelShader);

	//Load our test textures
	const std::string filepath("assets/textures/");
	int i = 0;
	for (auto& JustTexture : JustTextures)
	{
		JustTexture.CreateFromBitmap(RBitmap::FromFile(filepath + Paintings[i]), DXGI_FORMAT_R8G8B8A8_UNORM);
		i++;
	}
	
}

void BasePassDrawInstancedMesh(RCommandListGraphics& CmdList, RDrawableMesh& drawable)
{
	CmdList.SetVertexBuffer(0, drawable.GpuData.VertexBufferPositions.GetVertexBufferView());
	CmdList.SetIndexBuffer(drawable.GpuData.IndexBuffer.GetIndexBufferView());

	//PER DRAW DESCRIPTORS, WRITE INTO ROOT SIG DIRECTLY WITHOUT DESC HEAP
	//VertexBufferRest, Instance MatBuffer, IndexToMaterialBuffer
	//D3D12_CPU_DESCRIPTOR_HANDLE InstanceSRVs[3] = { drawable.GpuData.VertexBufferRest.GetSRV(), drawable.GpuData.AllInstancesModelMatricesGPU.GetSRV(), drawable.GpuData.AllInstancesIndicesToMaterialsGPU.GetSRV() };
	//CmdList.SetDynamicDescriptors(1, 0, 3, InstanceSRVs);
	CmdList.SetBufferSRV(1, drawable.GpuData.VertexBufferRest);
	CmdList.SetBufferSRV(2, drawable.GpuData.AllInstancesModelMatricesGPU);
	CmdList.SetBufferSRV(3, drawable.GpuData.AllInstancesIndicesToMaterialsGPU);

	CmdList.SetConstants(5, 0, 0, 0, 0);

	RMeshDesc MeshDesc = drawable.GeometryBufferCPU.GetMeshDesc();

	CmdList.DrawIndexedInstanced(MeshDesc.NumofIndices, drawable.GetInstanceCount(), MeshDesc.FirstIndexPos, MeshDesc.FirstVertexPos, 0);
}

RBasePass::Output RBasePass::Execute(Input& input)
{

	RCommandListGraphics& CmdList = RCommandList::BeginNew(L"BasePass").GetGraphicsContext();

	//Set global states 
	CmdList.SetViewport(SRenderer::Get().GetViewport());
	CmdList.SetScissor(SRenderer::Get().GetScissorRect());

	auto& RT = *input.RenderTargetColor;
	auto& DSV = *input.DepthTarget;

	/*CmdList.TransitionResource(RT, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	CmdList.TransitionResource(DSV, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);*/
	
	CmdList.SetRenderTarget(RT.GetRTV(), DSV.GetDSV());

	/*CmdList.ClearRenderTarget(RT);
	CmdList.ClearDepth(DSV);*/

	//Prepare Draw
	CmdList.SetRootSignature(RootSignature);

	CmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Uniform Buffer
	CmdList.SetDynamicDescriptor(0, 0, input.scene->SceneUniformBuffer.GetView());

	//Materials Buffer, Lights Buffer
	D3D12_CPU_DESCRIPTOR_HANDLE MatLightsSRV[2] = { RCommonGpuResourcesS::Get().LightsBuffer.GetSRV(),  RMaterialManagerS::GetSRV() };
	CmdList.SetDynamicDescriptors(4, 0, 2, MatLightsSRV);

	//TexturesSRV
	/*D3D12_CPU_DESCRIPTOR_HANDLE TextureSRV[7];
	for (int i = 0; i < 7; i++)
	{
		TextureSRV[i] = SLLRenderer::GetTexturesDescHandleCPU(i);
	}
	CmdList.SetDynamicDescriptors(3, 0, 7, TextureSRV);*/
	//CmdList.SetDynamicDescriptor(3, 0, SLLRenderer::GetTexturesDescHandleCPU(0));

	//Draw Simple Opaque
	//CmdList.SetPipelineState(BasePassCommonPSOs.Get(ECommonPSOType::PSODefault).PSO);
	CmdList.SetPipelineState(RPSOFactory::GetGraphicsPSO(RootSignature, VertexShader, PixelShader));

	for (int d = 0; d < input.scene->DefaultDrawables.size(); d++)
	{
		auto& drawable = input.scene->DefaultDrawables[d];

		BasePassDrawInstancedMesh(CmdList, drawable);
	}

	//Draw Visible LightSources
	//Point Light
	//BasePassDrawInstancedMesh(CmdList, RLightSource::PointLightDrawable);

	//Finish Draw
	CmdList.TransitionResource(RT, D3D12_RESOURCE_STATE_PRESENT);

	//Execute
	CmdList.ExecuteCmdListAndReleaseContext(false);

	Output output;
	output.DepthTarget = input.DepthTarget;
	output.RenderTargetColor = input.RenderTargetColor;

	return output;
}
