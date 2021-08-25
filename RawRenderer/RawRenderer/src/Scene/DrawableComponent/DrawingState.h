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

	RPipelineStateGraphics GetGraphicsPSO(RRootSignature& RootSignature, RShader VertexShader, RShader PixelShader, CommonPipelineStates commonPipelineStates = CommonPipelineStates{});
}