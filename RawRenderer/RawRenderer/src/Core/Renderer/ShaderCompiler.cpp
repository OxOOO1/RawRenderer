#include "ShaderCompiler.h"

#include "LLRenderer.h"

#include "Utilities/OutputPrint.h"


std::string convertToUTF8(const std::wstring& wstr)
{
	const int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	const std::unique_ptr<char[]> buffer(new char[bufferSize]);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.get(), bufferSize, nullptr, nullptr);
	return std::string(buffer.get());
}

std::wstring convertToUTF16(const std::string& str)
{
	const int bufferSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	const std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferSize]);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.get(), bufferSize);
	return std::wstring(buffer.get());
}


Microsoft::WRL::ComPtr<ID3DBlob> RShaderCompiler::CompileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile, std::vector<D3D_SHADER_MACRO>& defines)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	flags |= D3DCOMPILE_DEBUG;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	flags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

	ComPtr<ID3DBlob> shader;
	ComPtr<ID3DBlob> errorBlob;

	PRINTOUT(L"Compiling shader: " << filename.c_str() << " " << entryPoint.c_str() << std::endl;)

	if(!defines.empty())
	{
		defines.push_back({ NULL, NULL });
	}

	HRESULT hr = D3DCompileFromFile(convertToUTF16(filename).c_str(), defines.empty() ? nullptr : defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(), flags, 0, &shader, &errorBlob);

	if (FAILED(hr))
	{
		std::string errorMsg = "Shader compilation failed: ";
		for (auto& define : defines)
		{
			if(define.Name)
			errorMsg = errorMsg + define.Name + " ";
		}
		if (errorBlob) {
			errorMsg += std::string("\n") + static_cast<const char*>(errorBlob->GetBufferPointer());
			PRINTOUT(errorMsg.c_str())
				errorBlob->Release();
		}
		else
			PRINTOUT(errorMsg.c_str() << " no debug info available")
			if (shader)
				shader->Release();
	}

	ASSERTHR(hr);

	return shader;
}