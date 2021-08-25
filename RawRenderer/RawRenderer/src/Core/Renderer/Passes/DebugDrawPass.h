#pragma once
#include <DirectXMath.h>
#include "thirdParty/d3dx12.h"
#include <d3d12.h>

#include "Resources/Texture/Texture.h"
#include "CommandContext.h"
#include "Scene/Scene.h"

#include "Resources/UniformBuffer.h"
#include "Resources/Shader.h"

#include "RenderPass.h"

class RDebugDrawPassRootSignature : public RRootSignature
{
public:
	RDebugDrawPassRootSignature()
	{
		Reset(2, 1);

		//Sampler
		CD3DX12_STATIC_SAMPLER_DESC DefaultSamplerDesc{ 0, D3D12_FILTER_MIN_MAG_MIP_LINEAR };
		InitStaticSampler(0, *reinterpret_cast<D3D12_SAMPLER_DESC*>(&DefaultSamplerDesc), D3D12_SHADER_VISIBILITY_PIXEL);

		//UniformBuffer
		GetRootParam(0).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

		//AllInstancesDescs Buffer
		GetRootParam(1).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
			//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
			;

		Finalize(L"DebugDrawPassRS", RootSignatureFlags);
	}
};

class RDebugDrawPassPixelShader : public RShader
{
public:
	RDebugDrawPassPixelShader()
	{
		CreatePS("shaders/DebugDrawPass.hlsl", "MainPS", "ps_5_1");
	}
};

class RBasePass : public RPass
{
public:

	void InitResources() override;

	struct Input
	{
		RScene* scene;
		RColorBuffer* RenderTargetColor;
		RDepthBuffer* DepthTarget;
	};
	struct Output
	{
		RColorBuffer* RenderTargetColor;
		RDepthBuffer* DepthTarget;
	};

	Output Execute(Input& input);

private:

	RDebugDrawPassRootSignature RootSignature;
	RDebugDrawPassPixelShader PixelShader;


};