#pragma once

#include "Core/Renderer/RootSignature.h"
#include "thirdParty/d3dx12.h"
#include "Resources/CommonResources.h"
#include "Resources/Shader.h"

class RPipelineState
{
public:

	RPipelineState() = default;

	static void DestroyAll(void);

	void SetRootSignature(const RRootSignature& BindMappings)
	{
		RootSignature = &BindMappings;
	}

	const RRootSignature& GetRootSignature(void) const
	{
		assert(RootSignature != nullptr);
		return *RootSignature;
	}

	ID3D12PipelineState* GetPipelineStateObject(void) const { return PSO; }

protected:

	const RRootSignature* RootSignature{ nullptr };

	ID3D12PipelineState* PSO;
};

class RPipelineStateGraphics : public RPipelineState
{
	friend class CommandContext;

public:

	// Start with empty state
	RPipelineStateGraphics()
	{
		ZeroMemory(&PSODesc, sizeof(PSODesc));
		PSODesc.NodeMask = 1;
		PSODesc.SampleMask = 0xFFFFFFFFu;
		PSODesc.SampleDesc.Count = 1;
		PSODesc.InputLayout.NumElements = 0;
	}

	void SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
	{
		PSODesc.BlendState = BlendDesc;
	}
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
	{
		PSODesc.RasterizerState = RasterizerDesc;
	}
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
	{
		PSODesc.DepthStencilState = DepthStencilDesc;
	}
	void SetSampleMask(UINT SampleMask)
	{
		PSODesc.SampleMask = SampleMask;
	}
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
	{
		assert(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
		PSODesc.PrimitiveTopologyType = TopologyType;
	}
	void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0)
	{
		SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
	}
	void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
	void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
	{
		PSODesc.IBStripCutValue = IBProps;
	}

	// These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
	void SetVertexShader(const void* Binary, size_t Size) { PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
	void SetPixelShader(const void* Binary, size_t Size) { PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
	void SetGeometryShader(const void* Binary, size_t Size) { PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
	void SetHullShader(const void* Binary, size_t Size) { PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
	void SetDomainShader(const void* Binary, size_t Size) { PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }

	void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.VS = Binary; }
	void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.PS = Binary; }
	void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.GS = Binary; }
	void SetHullShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.HS = Binary; }
	void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.DS = Binary; }

	// Perform validation and compute a hash value for fast state block comparisons
	void InitGetOrCreate();

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
	std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> InputLayouts;

};

