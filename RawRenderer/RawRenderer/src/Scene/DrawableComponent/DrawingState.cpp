#include "DrawingState.h"

RPipelineStateGraphics RPSOFactory::GetGraphicsPSO(RRootSignature& RootSignature, RShader VertexShader, RShader PixelShader, CommonPipelineStates commonPipelineStates)
{
	RPipelineStateGraphics PSO;

	//1)Input 
	PSO.SetRootSignature(RootSignature); //Based on Pass

										 //2)Geometry State
	PSO.SetVertexShader(VertexShader.Get()); //Based on Mesh

	PSO.SetInputLayout(RVertexComponentPositionOnly::NumAttributes, RVertexComponentPositionOnly::InputElementDesc);
	PSO.SetPrimitiveTopologyType(commonPipelineStates.TopologyType);

	PSO.SetDepthStencilState(commonPipelineStates.DepthStencilDesc);

	//3)Material State
	PSO.SetPixelShader(PixelShader.Get());
	//commonPipelineStates.RasterDesc.CullMode = D3D12_CULL_MODE_NONE;
	PSO.SetRasterizerState(commonPipelineStates.RasterDesc); //Based on Material;
	PSO.SetBlendState(commonPipelineStates.BlendDesc); //Based on Material

	PSO.SetRenderTargetFormat(RCommonGpuResourcesS::Get().SceneColor.GetFormat(), RCommonGpuResourcesS::Get().SceneDepth.GetFormat()); //RT

	PSO.InitGetOrCreate();

	return PSO;
}
