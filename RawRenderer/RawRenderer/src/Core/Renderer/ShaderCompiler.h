#pragma once

#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3d12.h>

std::string convertToUTF8(const std::wstring& wstr);

std::wstring convertToUTF16(const std::string& str);

namespace RShaderCompiler
{
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile, std::vector<D3D_SHADER_MACRO>& defines);
}