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

class RBasePassRootSignature : public RRootSignature
{
public:
	RBasePassRootSignature()
	{
		Reset(6, 1);

		//Sampler
		CD3DX12_STATIC_SAMPLER_DESC DefaultSamplerDesc{ 0, D3D12_FILTER_MIN_MAG_MIP_LINEAR };
		InitStaticSampler(0, *reinterpret_cast<D3D12_SAMPLER_DESC*>(&DefaultSamplerDesc), D3D12_SHADER_VISIBILITY_PIXEL);

		//UniformBuffer DescTable
		GetRootParam(0).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

		//VertexBufferRest, AllInstancesModelMatrices, AllInstancesIndicesToMaterials
		//GetRootParam(1).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_VERTEX);
		GetRootParam(1).InitAsBufferSRV(0, D3D12_SHADER_VISIBILITY_VERTEX);
		GetRootParam(2).InitAsBufferSRV(1, D3D12_SHADER_VISIBILITY_VERTEX);
		GetRootParam(3).InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_VERTEX);

		//LightsBuffer, MaterialDescsBuffer
		GetRootParam(4).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);

		//TextureSRV
		//GetRootParam(3).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 7, D3D12_SHADER_VISIBILITY_PIXEL);
		//GetRootParam(3).InitAsDescriptorTable(1, D3D12_SHADER_VISIBILITY_PIXEL);
		//GetRootParam(3).SetTableRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5);

		//Root Constants
		GetRootParam(5).InitAsConstants(1, 4, D3D12_SHADER_VISIBILITY_PIXEL);

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
			//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
			;

		Finalize(L"BasePassRS", RootSignatureFlags);
	}
};

class RBasePassVertexShader : public RShader
{
public:
	RBasePassVertexShader()
	{
		PushBackDefine({ "VERTEX_LAYOUT_POSITION", "1" });
		CreateVS("shaders/BasePass.hlsl", "MainVS", "vs_5_1");
	}
};
class RBasePassPixelShader : public RShader 
{
public:
	RBasePassPixelShader()
	{
		PushBackDefine({ "VERTEX_LAYOUT_POSITION", "1" });
		CreatePS("shaders/BasePass.hlsl", "MainPS", "ps_5_1");
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

	RBasePassRootSignature RootSignature;
	RBasePassVertexShader VertexShader;
	RBasePassPixelShader PixelShader;


	std::array<RTexture, 7> JustTextures;
	std::array<std::string, 7> Paintings =
	{
		"girl.jpg",
		"guernica.jpg",
		"lisa.jpg",
		"rembr.jpg",
		"scream.jpg",
		"vangog.jpg",
		"venus.jpg",
	};
};
