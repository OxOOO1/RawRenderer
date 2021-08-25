#pragma once

#include "Resources/ColorBuffer.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "Resources/CommonResources.h"
#include "Resources/Texture/Texture.h"
#include "CommandContext.h"
#include "Scene/Scene.h"

#include "Resources/UniformBuffer.h"
#include "Resources/Shader.h"

class RBeginPresentPassRootSignature : public RRootSignature
{
public:
	RBeginPresentPassRootSignature()
	{
		Reset(1, 1);
		GetRootParam(0).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		//RootSignature[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
		//RootSignature[0].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
		//RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		//RootSignature.InitStaticSampler(0, SCommonResources::Get().SamplerPointClampDesc);
		InitStaticSampler(0, RCommonGpuResourcesS::Get().SamplerLinearClampDesc);
		Finalize(L"BeginPresent");
	}
};

class RBeginPresentPassVertexShader : public RShader
{
public:
	RBeginPresentPassVertexShader()
	{
		Create("shaders/UpscalePresent.hlsl", "MainVS", "vs_5_0");
	}
};

class RBeginPresentPassPixelShader : public RShader 
{
public:
	RBeginPresentPassPixelShader()
	{
		PushBackDefine({ "COLOR_FORMAT", "0" });
		PushBackDefine({ "BILLINEAR_UPSCALE", "1" });
		Create("shaders/UpscalePresent.hlsl", "MainPS", "ps_5_0");
	}
};


class RBeginPresentPass
{
public:

	struct Input
	{
		RColorBuffer* ColorRTMain;
	};

	struct Output
	{
		RColorBuffer* CurrentSwapchainBackBuffer;
	};

	enum EUpscaleFilterType
	{
		Bilinear = 0,
		Bicubic,
		Sharpening
	};
	EUpscaleFilterType UpscaleFilter = Bilinear;

	Output Execute(Input& input);

	void InitResources();

private:

	RBeginPresentPassRootSignature RootSignature;

	RBeginPresentPassVertexShader VertexShader;
	RBeginPresentPassPixelShader PixelShader;

	RPipelineStateGraphics PSODefault;
	RPipelineStateGraphics PSOBillinear;

};