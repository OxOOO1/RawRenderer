#pragma once

#include "Renderer/PipelineState.h"

#include "VertexComponent.h"

namespace RPSOFactory
{
	struct CommonPipelineStates
	{
		D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC{ CD3DX12_DEFAULT{} };
		D3D12_BLEND_DESC BlendDesc = CD3DX12_BLEND_DESC{ CD3DX12_DEFAULT{} };
		D3D12_RASTERIZER_DESC RasterDesc = CD3DX12_RASTERIZER_DESC{ CD3DX12_DEFAULT{} };
		D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	};

	RPipelineStateGraphics GetGraphicsPSO(RRootSignature& RootSignature, RShader VertexShader, RShader PixelShader, CommonPipelineStates commonPipelineStates = CommonPipelineStates{})
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
}