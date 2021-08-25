#include "RootSignature.h"

#include "Core/Renderer/LLRenderer.h"
#include <mutex>

std::map<size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> RRootSignature::RootSignatureHashMap;

void RRootSignature::DestroyAll(void)
{
	RootSignatureHashMap.clear();
}

void RRootSignature::InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
	assert(NumInitializedStaticSamplers < NumSamplers);
	D3D12_STATIC_SAMPLER_DESC& StaticSamplerDesc = StaticSamplersArr[NumInitializedStaticSamplers++];

	StaticSamplerDesc.Filter = NonStaticSamplerDesc.Filter;
	StaticSamplerDesc.AddressU = NonStaticSamplerDesc.AddressU;
	StaticSamplerDesc.AddressV = NonStaticSamplerDesc.AddressV;
	StaticSamplerDesc.AddressW = NonStaticSamplerDesc.AddressW;
	StaticSamplerDesc.MipLODBias = NonStaticSamplerDesc.MipLODBias;
	StaticSamplerDesc.MaxAnisotropy = NonStaticSamplerDesc.MaxAnisotropy;
	StaticSamplerDesc.ComparisonFunc = NonStaticSamplerDesc.ComparisonFunc;
	StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	StaticSamplerDesc.MinLOD = NonStaticSamplerDesc.MinLOD;
	StaticSamplerDesc.MaxLOD = NonStaticSamplerDesc.MaxLOD;
	StaticSamplerDesc.ShaderRegister = Register;
	StaticSamplerDesc.RegisterSpace = 0;
	StaticSamplerDesc.ShaderVisibility = Visibility;

	if (StaticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
		StaticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
		StaticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
	{
		assert(
			// Transparent Black
			NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
			NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
			NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
			NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
			// Opaque Black
			NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
			NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
			NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
			NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
			// Opaque White
			NonStaticSamplerDesc.BorderColor[0] == 1.0f &&
			NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
			NonStaticSamplerDesc.BorderColor[2] == 1.0f &&
			NonStaticSamplerDesc.BorderColor[3] == 1.0f,
			"Sampler border color does not match static sampler limitations");

		if (NonStaticSamplerDesc.BorderColor[3] == 1.0f)
		{
			if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
				StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			else
				StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		}
		else
			StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	}
}

void RRootSignature::Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags /*= D3D12_ROOT_SIGNATURE_FLAG_NONE*/)
{
	if (Finalized)
		return;

	assert(NumInitializedStaticSamplers == NumSamplers);

	D3D12_ROOT_SIGNATURE_DESC RootDesc;
	RootDesc.NumParameters = NumParameters;
	RootDesc.pParameters = (const D3D12_ROOT_PARAMETER*)RootParamsArr.get();
	RootDesc.NumStaticSamplers = NumSamplers;
	RootDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)StaticSamplersArr.get();
	RootDesc.Flags = Flags;

	DescriptorTableBitMap = 0;
	SamplerTableBitMap = 0;

	size_t HashCode = RHash::HashState(&RootDesc.Flags);
	HashCode = RHash::HashState(RootDesc.pStaticSamplers, NumSamplers, HashCode);

	for (UINT Param = 0; Param < NumParameters; ++Param)
	{
		const D3D12_ROOT_PARAMETER& RootParam = RootDesc.pParameters[Param];
		DescriptorTableSize[Param] = 0;

		if (RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			assert(RootParam.DescriptorTable.pDescriptorRanges != nullptr);

			HashCode = RHash::HashState(RootParam.DescriptorTable.pDescriptorRanges,
				RootParam.DescriptorTable.NumDescriptorRanges, HashCode);

			// We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
			if (RootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
				SamplerTableBitMap |= (1 << Param);
			else
				DescriptorTableBitMap |= (1 << Param);

			for (UINT TableRange = 0; TableRange < RootParam.DescriptorTable.NumDescriptorRanges; ++TableRange)
				DescriptorTableSize[Param] += RootParam.DescriptorTable.pDescriptorRanges[TableRange].NumDescriptors;
		}
		else
			HashCode = RHash::HashState(&RootParam, 1, HashCode);
	}

	ID3D12RootSignature** RSRef = nullptr;
	bool firstCompile = false;
	{
		static std::mutex sHashMapMutex;
		std::lock_guard<std::mutex> CS(sHashMapMutex);
		auto iter = RootSignatureHashMap.find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == RootSignatureHashMap.end())
		{
			RSRef = RootSignatureHashMap[HashCode].GetAddressOf();
			firstCompile = true;
		}
		else
			RSRef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

		ASSERTHR(D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf()));

		ASSERTHR(SLLRenderer::GetDevice()->CreateRootSignature(1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
			IID_PPV_ARGS(&Signature)));

		Signature->SetName(name.c_str());

		RootSignatureHashMap[HashCode].Attach(Signature);
		assert(*RSRef == Signature);
	}
	else
	{
		while (*RSRef == nullptr)
			std::this_thread::yield();
		Signature = *RSRef;
	}

	Finalized = TRUE;
}

