#include "UniformBuffer.h"

#include "LLRenderer.h"
#include "Scene/Scene.h"

void RUniformBuffer::Create(UINT size)
{
	const size_t AlignmentMask = DEFAULT_ALIGN - 1;

	// Assert that it's a power of two.
	assert((AlignmentMask & DEFAULT_ALIGN) == 0);

	Size = RMath::AlignUpWithMask(size, AlignmentMask);
	Size = Size < kGpuAllocatorPageSize ? kGpuAllocatorPageSize : Size;

	//Create Default buffer
	UsageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

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
	ResourceDesc.Width = Size;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERTHR(
		SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
			&ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&Resource)));

	GpuVirtualAddress = Resource->GetGPUVirtualAddress();

#ifdef _DEBUG
	Resource->SetName(L"Uniform Buffer");
#endif

	//Create CBuffer view
	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GpuVirtualAddress;
	CBVDesc.SizeInBytes = Size;

	ViewHandleCPU = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	SLLRenderer::GetDevice()->CreateConstantBufferView(&CBVDesc, ViewHandleCPU);
}
