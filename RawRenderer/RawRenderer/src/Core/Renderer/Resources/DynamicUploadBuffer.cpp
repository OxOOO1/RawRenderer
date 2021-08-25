#include "DynamicUploadBuffer.h"

#include "LLRenderer.h"

void RUploadBufferDynamic::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize)
{
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Width = NumElements * ElementSize;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERTHR(SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Resource)));

	Resource->SetName(name.c_str());

	GpuVirtualAddress = Resource->GetGPUVirtualAddress();
	CpuVirtualAddress = nullptr;
}

void* RUploadBufferDynamic::Map(void)
{
	assert(CpuVirtualAddress == nullptr, "Buffer is already locked");
	ASSERTHR(Resource->Map(0, nullptr, &CpuVirtualAddress));
	return CpuVirtualAddress;
}

void RUploadBufferDynamic::Unmap(void)
{
	assert(CpuVirtualAddress != nullptr, "Buffer is not locked");
	Resource->Unmap(0, nullptr);
	CpuVirtualAddress = nullptr;
}
