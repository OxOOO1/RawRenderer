#pragma once

#include "Resources/Shader.h"
#include <DirectXMath.h>
#include "Vertex.h"

#include "shaders/VertexInput.h"	

constexpr DXGI_FORMAT DFloat3 = DXGI_FORMAT_R32G32B32_FLOAT;
constexpr DXGI_FORMAT DFloat2 = DXGI_FORMAT_R32G32_FLOAT;
#define DECL_ATTR(format, name) { #name,  0, format, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

using namespace DirectX;

class RVertexComponentPositionOnly
{
public:
	static constexpr UINT NumAttributes = 1;
	static constexpr D3D12_INPUT_ELEMENT_DESC InputElementDesc[NumAttributes] =
	{
		DECL_ATTR(DFloat3, Position)
	};

	/*static RShader CreateVertexShader()
	{
		RShader vertexShader;
		vertexShader.PushBackDefine({ "VERTEX_LAYOUT_POSITION", "1" });
		vertexShader.CreateVS("shaders/BasePass.hlsl", "MainVS", "vs_5_1");
		return vertexShader;
	}*/

};

class RVertexComponentDefault
{
public:
	struct Vertex
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT3 Tangent;
		XMFLOAT3 Bitangent;
		XMFLOAT2 TexCoord;
	};
	static constexpr UINT NumAttributes = 5;
	static constexpr D3D12_INPUT_ELEMENT_DESC InputElementDesc[NumAttributes] =
	{
		DECL_ATTR(DFloat3, Position)
		DECL_ATTR(DFloat3, Normal)
		DECL_ATTR(DFloat3, Tangent)
		DECL_ATTR(DFloat3, Bitangent)
		DECL_ATTR(DFloat2, TexCoord)
	};
	static constexpr RVertexAttribute::EAttribType Attributes[NumAttributes] =
	{
		RVertexAttribute::EAttribType::Position3D,
		RVertexAttribute::EAttribType::Normal,
		RVertexAttribute::EAttribType::Tangent,
		RVertexAttribute::EAttribType::Bitangent,
		RVertexAttribute::EAttribType::TexCoord,
	};

	static RShader CreateVertexShader()
	{	
		RShader vertexShader;
		vertexShader.PushBackDefine({ "VERTEX_LAYOUT_DEFAULT", "1" });
		vertexShader.CreateVS("shaders/BasePass.hlsl", "MainVS", "vs_5_1");
		return vertexShader;
	}

};

