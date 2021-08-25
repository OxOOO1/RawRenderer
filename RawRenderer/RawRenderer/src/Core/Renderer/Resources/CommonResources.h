#pragma once

#include "DepthBuffer.h"
#include "ColorBuffer.h"

#include "Utilities/Types.h"
#include "thirdParty/d3dx12.h"

#include "Scene/LightComponent/Lights.h"

class RCommonGpuResourcesS
{

	friend class SRenderer;
public:

	static constexpr DXGI_FORMAT RTFormat = DXGI_FORMAT_R11G11B10_FLOAT;
	static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;

	static RCommonGpuResourcesS& Get()
	{
		assert(pSingleton);
		return *pSingleton;
	}

public:
	////////////////////////////////////////////////////////
	//					Scene Render Targets
	////////////////////////////////////////////////////////

	RColorBuffer SceneColor;
	RDepthBuffer SceneDepth{ 1.f };

	////////////////////////////////////////////////////////
	//					Descriptors
	////////////////////////////////////////////////////////

	//Samplers
	CD3DX12_STATIC_SAMPLER_DESC SamplerPointClampDesc{0, D3D12_FILTER_MIN_MAG_MIP_POINT };
	CD3DX12_STATIC_SAMPLER_DESC SamplerLinearClampDesc{0, D3D12_FILTER_MIN_MAG_MIP_LINEAR };

	//Rasterizers
	D3D12_RASTERIZER_DESC RasterizerDefault{ CD3DX12_RASTERIZER_DESC{CD3DX12_DEFAULT{}} };
	D3D12_RASTERIZER_DESC RasterizerTwoSided;
	D3D12_RASTERIZER_DESC RasterizerShadow;

	//Blenders
	D3D12_BLEND_DESC BlendDefault{ CD3DX12_BLEND_DESC{CD3DX12_DEFAULT{}} };
	D3D12_BLEND_DESC BlendNoColorWrite;
	D3D12_BLEND_DESC BlendDisable;
	D3D12_BLEND_DESC BlendPreMultiplied;
	D3D12_BLEND_DESC BlendTraditional;
	D3D12_BLEND_DESC BlendAdditive;
	D3D12_BLEND_DESC BlendTraditionalAdditive;

	//DepthStencilStates
	D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
	D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
	D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
	D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
	D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

	////////////////////////////////////////////////////////
	//					Common Buffers
	////////////////////////////////////////////////////////

	RStructuredBuffer LightsBuffer;
	RStructuredBuffer AllMaterialsDescriptorsBuffer;

private:
	RCommonGpuResourcesS(UINT2 RTSize);

	~RCommonGpuResourcesS()
	{
		SceneColor.Destroy();
		SceneDepth.Destroy();
	}

	void InitDescs()
	{
		//Raster
		RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
		RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
		RasterizerDefault.FrontCounterClockwise = TRUE;
		RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		RasterizerDefault.DepthClipEnable = TRUE;
		RasterizerDefault.MultisampleEnable = FALSE;
		RasterizerDefault.AntialiasedLineEnable = FALSE;
		RasterizerDefault.ForcedSampleCount = 0;
		RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		/*RasterizerDefaultMsaa = RasterizerDefault;
		RasterizerDefaultMsaa.MultisampleEnable = TRUE;

		RasterizerDefaultCw = RasterizerDefault;
		RasterizerDefaultCw.FrontCounterClockwise = FALSE;

		RasterizerDefaultCwMsaa = RasterizerDefaultCw;
		RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;*/

		RasterizerTwoSided = RasterizerDefault;
		RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

		/*RasterizerTwoSidedMsaa = RasterizerTwoSided;
		RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;*/

		// Shadows need their own rasterizer state so we can reverse the winding of faces
		RasterizerShadow = RasterizerDefault;
		//RasterizerShadow.CullMode = D3D12_CULL_FRONT;  // Hacked here rather than fixing the content
		RasterizerShadow.SlopeScaledDepthBias = -1.5f;
		RasterizerShadow.DepthBias = -100;

		/*RasterizerShadowTwoSided = RasterizerShadow;
		RasterizerShadowTwoSided.CullMode = D3D12_CULL_MODE_NONE;

		RasterizerShadowCW = RasterizerShadow;
		RasterizerShadowCW.FrontCounterClockwise = FALSE;*/

		//Depth
		DepthStateDisabled.DepthEnable = FALSE;
		DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		DepthStateDisabled.StencilEnable = FALSE;
		DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		DepthStateDisabled.BackFace = DepthStateDisabled.FrontFace;

		DepthStateReadWrite = DepthStateDisabled;
		DepthStateReadWrite.DepthEnable = TRUE;
		DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

		DepthStateReadOnly = DepthStateReadWrite;
		DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		DepthStateReadOnlyReversed = DepthStateReadOnly;
		DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

		DepthStateTestEqual = DepthStateReadOnly;
		DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

		//Blend
		D3D12_BLEND_DESC alphaBlend = {};
		alphaBlend.IndependentBlendEnable = FALSE;
		alphaBlend.RenderTarget[0].BlendEnable = FALSE;
		alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		alphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		alphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		alphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
		BlendNoColorWrite = alphaBlend;

		alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		BlendDisable = alphaBlend;

		alphaBlend.RenderTarget[0].BlendEnable = TRUE;
		BlendTraditional = alphaBlend;

		alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		BlendPreMultiplied = alphaBlend;

		alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		BlendAdditive = alphaBlend;

		alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		BlendTraditionalAdditive = alphaBlend;

	}

private:

	static RCommonGpuResourcesS* pSingleton;


};

