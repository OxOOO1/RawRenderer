#pragma once

#include <d3d12.h>
#include "thirdParty/d3dx12.h"
#include <cstdint>
#include <memory>
#include <map>
#include <wrl/client.h>
#include <cassert>
#include <string>
#include "Utilities/Hash.h"

class RRootParameter
{

	friend class RRootSignature;

public:

	RRootParameter() = default;

	~RRootParameter()
	{
		Clear();
	}

	void Clear()
	{
		if (RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			delete[] RootParam.DescriptorTable.pDescriptorRanges;

		RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
	}

	void InitAsConstants(UINT Register, UINT NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		RootParam.ShaderVisibility = Visibility;
		RootParam.Constants.Num32BitValues = NumDwords;
		RootParam.Constants.ShaderRegister = Register;
		RootParam.Constants.RegisterSpace = 0;
	}

	void InitAsConstantBuffer(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		RootParam.ShaderVisibility = Visibility;
		RootParam.Descriptor.ShaderRegister = Register;
		RootParam.Descriptor.RegisterSpace = 0;
	}

	void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		RootParam.ShaderVisibility = Visibility;
		RootParam.Descriptor.ShaderRegister = Register;
		RootParam.Descriptor.RegisterSpace = 0;
	}

	void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		RootParam.ShaderVisibility = Visibility;
		RootParam.Descriptor.ShaderRegister = Register;
		RootParam.Descriptor.RegisterSpace = 0;
	}

	void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsDescriptorTable(1, Visibility);
		SetTableRange(0, Type, Register, Count);
	}

	void InitAsDescriptorTable(UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParam.ShaderVisibility = Visibility;
		RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
		RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
	}

	void SetTableRange(UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0)
	{
		D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
		range->RangeType = Type;
		range->NumDescriptors = Count;
		range->BaseShaderRegister = Register;
		range->RegisterSpace = Space;
		range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	const D3D12_ROOT_PARAMETER& Get() const
	{
		return RootParam;
	}

protected:

	D3D12_ROOT_PARAMETER RootParam{ (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF };

};

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)

class RRootSignature
{

	friend class RDynamicDescriptorHeap;
	friend struct DescriptorHandleCache;

public:

	RRootSignature(UINT NumRootParams = 0, UINT NumStaticSamplers = 0) : NumParameters(NumRootParams)
	{
		Reset(NumRootParams, NumStaticSamplers);
	}

	static void DestroyAll(void);

	void Reset(UINT NumRootParams, UINT NumStaticSamplers = 0)
	{
		if (NumRootParams > 0)
			RootParamsArr.reset(new RRootParameter[NumRootParams]);
		else
			RootParamsArr = nullptr;

		NumParameters = NumRootParams;

		if (NumStaticSamplers > 0)
			StaticSamplersArr.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
		else
			StaticSamplersArr = nullptr;
		NumSamplers = NumStaticSamplers;
		NumInitializedStaticSamplers = 0;
	}

	RRootParameter& GetRootParam (size_t EntryIndex)
	{
		assert(EntryIndex < NumParameters);
		return RootParamsArr.get()[EntryIndex];
	}

	const RRootParameter& GetRootParam (size_t EntryIndex) const
	{
		assert(EntryIndex < NumParameters);
		return RootParamsArr.get()[EntryIndex];
	}

	RRootParameter& operator[](size_t EntryIndex)
	{
		assert(EntryIndex < NumParameters);
		return RootParamsArr.get()[EntryIndex];
	}
	const RRootParameter& operator[](size_t EntryIndex) const
	{
		assert(EntryIndex < NumParameters);
		return RootParamsArr.get()[EntryIndex];
	}

	void InitStaticSampler(UINT Register, const CD3DX12_STATIC_SAMPLER_DESC& NonStaticSamplerDesc,
		D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitStaticSampler(Register, (const D3D12_SAMPLER_DESC&)NonStaticSamplerDesc,
			Visibility);
	}
	void InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
		D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);

	void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ID3D12RootSignature* GetSignature() const { return Signature; }

protected:

	BOOL Finalized = false;
	UINT NumParameters;
	UINT NumSamplers;
	UINT NumInitializedStaticSamplers;

	uint32_t DescriptorTableBitMap;        // One bit is set for root parameters that are non-sampler descriptor tables
	uint32_t SamplerTableBitMap;            // One bit is set for root parameters that are sampler descriptor tables

	uint32_t DescriptorTableSize[16];        // Non-sampler descriptor tables need to know their descriptor count

	std::unique_ptr<RRootParameter[]> RootParamsArr;
	std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> StaticSamplersArr;

	ID3D12RootSignature* Signature;

	static std::map<size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> RootSignatureHashMap;
};
