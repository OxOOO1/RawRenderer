#include "Sampler.h"

#include <map>
#include "LLRenderer.h"


namespace
{
	std::map< size_t, D3D12_CPU_DESCRIPTOR_HANDLE > SamplerCache;
}

D3D12_CPU_DESCRIPTOR_HANDLE RSamplerDesc::CreateDescriptor(void)
{
	size_t hashValue = RHash::HashState(this);
	auto iter = SamplerCache.find(hashValue);
	if (iter != SamplerCache.end())
	{
		return iter->second;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Handle = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SLLRenderer::GetDevice()->CreateSampler(this, Handle);
	return Handle;
}

void RSamplerDesc::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& Handle)
{
	SLLRenderer::GetDevice()->CreateSampler(this, Handle);
}
