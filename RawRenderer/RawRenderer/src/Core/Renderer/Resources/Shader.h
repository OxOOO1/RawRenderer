#pragma once

#include "ShaderCompiler.h"
#include "thirdParty/d3dx12.h"

class RShader
{
public:

	RShader() = default;

	RShader(const std::string& FileName, const std::string& Main, const std::string& Version)
	{
		Create(FileName, Main, Version);
	}

	void Create(const std::string& FileName, const std::string& Main, const std::string& Version)
	{
		Bytecode = RShaderCompiler::CompileShader(FileName, Main, Version, Defines);
	}

	void CreateVS(const std::string& FileName, const std::string& Main, const std::string& Version)
	{	
		PushBackDefine({ "VERTEX_SHADER", "1" });
		Bytecode = RShaderCompiler::CompileShader(FileName, Main, Version, Defines);
	}

	void CreatePS(const std::string& FileName, const std::string& Main, const std::string& Version)
	{
		PushBackDefine({ "PIXEL_SHADER", "1" });
		Bytecode = RShaderCompiler::CompileShader(FileName, Main, Version, Defines);
	}

	//D3D_SHADER_MACRO ExampleDefine{ "MyDefine", "1" };
	void PushBackDefine(D3D_SHADER_MACRO define)
	{
		Defines.push_back(define);
	}

	D3D12_SHADER_BYTECODE Get()
	{
		return CD3DX12_SHADER_BYTECODE(Bytecode.Get());
	}

	Microsoft::WRL::ComPtr<ID3DBlob> Bytecode;
	//D3D12_SHADER_BYTECODE Bytecode;
	std::vector<D3D_SHADER_MACRO> Defines{};

};
