#include "PipelineState.h"

#include <mutex>

#include "Core/Renderer/LLRenderer.h"

static std::map< size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState> > GraphicsPSOHashMap;
static std::map< size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState> > ComputePSOHashMap;

void RPipelineStateGraphics::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount /*= 1*/, UINT MsaaQuality /*= 0*/)
{
	assert(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");

	for (UINT i = 0; i < NumRTVs; ++i)
		PSODesc.RTVFormats[i] = RTVFormats[i];

	for (UINT i = NumRTVs; i < PSODesc.NumRenderTargets; ++i)
		PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

	PSODesc.NumRenderTargets = NumRTVs;
	PSODesc.DSVFormat = DSVFormat;
	PSODesc.SampleDesc.Count = MsaaCount;
	PSODesc.SampleDesc.Quality = MsaaQuality;
	PSODesc.SampleMask = UINT_MAX;
}

void RPipelineStateGraphics::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{
	PSODesc.InputLayout.NumElements = NumElements;

	if (NumElements > 0)
	{
		D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
		memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
		InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
	}
	else
		InputLayouts = nullptr;
}

void RPipelineStateGraphics::InitGetOrCreate()
{
	// Make sure the root signature is finalized first
	PSODesc.pRootSignature = RootSignature->GetSignature();
	assert(PSODesc.pRootSignature != nullptr);

	PSODesc.InputLayout.pInputElementDescs = nullptr;
	size_t HashCode = RHash::HashState(&PSODesc);
	HashCode = RHash::HashState(InputLayouts.get(), PSODesc.InputLayout.NumElements, HashCode);
	PSODesc.InputLayout.pInputElementDescs = InputLayouts.get();

	ID3D12PipelineState** PSORef = nullptr;
	bool firstCompile = false;
	{
		static std::mutex s_HashMapMutex;
		std::lock_guard<std::mutex> CS(s_HashMapMutex);
		auto iter = GraphicsPSOHashMap.find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == GraphicsPSOHashMap.end())
		{
			firstCompile = true;
			PSORef = GraphicsPSOHashMap[HashCode].GetAddressOf();
		}
		else
			PSORef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		ASSERTHR(SLLRenderer::GetDevice()->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));
		GraphicsPSOHashMap[HashCode].Attach(PSO);
	}
	else
	{
		while (*PSORef == nullptr)
			std::this_thread::yield();
		PSO = *PSORef;
	}
}

void RPipelineState::DestroyAll(void)
{
	GraphicsPSOHashMap.clear();
	ComputePSOHashMap.clear();
}
