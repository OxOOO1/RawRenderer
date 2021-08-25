#include "DrawingState.h"



RCommonPSOs RCommonPSOsManagerS::InitCommonPSOsImpl(RRootSignature& rootsignature, RShader& pixelshader)
{
	RCommonPSOs outPSOs;
	RCommonPSO::CommonPipelineStates PipelineState;
	//PipelineState.RasterDesc.CullMode = D3D12_CULL_MODE_BACK;
	//PipelineState.DepthStencilDesc.DepthEnable = FALSE;
	PipelineState.DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	outPSOs.PSODefault.Create<RVertexComponentPositionOnly>(rootsignature, pixelshader, PipelineState);
	//DrawingStateDefaultPositionOnly.Create<RVertexComponentPositionOnly>(rootsignature, pixelshader, PipelineState);

	//DrawingStateDefaultTwoSided.Create<RVertexComponentDefault>(rootsignature, pixelshader, PipelineState);
	//DrawingStateDefaultPositionOnlyTwoSided.Create<RVertexComponentPositionOnly>(rootsignature, pixelshader, PipelineState);

	//PipelineState.RasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//DrawingStateDefaultWireframe.Create<RVertexComponentDefault>(rootsignature, pixelshader, PipelineState);
	//DrawingStatePositionOnlyWireframe.Create<RVertexComponentPositionOnly>(rootsignature, pixelshader, PipelineState);
	return outPSOs;
}
