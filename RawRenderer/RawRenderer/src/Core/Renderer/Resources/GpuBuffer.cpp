#include "GpuBuffer.h"

#include "LLRenderer.h"

#include "Scene/DrawableComponent/Vertex.h"

void RGpuBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t inElementSize, const void* initialData /*= nullptr*/)
{
	Destroy();

	ElementCount = NumElements;
	ElementSize = inElementSize;
	BufferSize = NumElements * ElementSize;

	D3D12_RESOURCE_DESC ResourceDesc = GetDesc();

	UsageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	ASSERTHR(
		SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
			&ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&Resource)));

	GpuVirtualAddress = Resource->GetGPUVirtualAddress();

	if (initialData)
		RCommandList::UploadToBufferImmediate(*this, initialData, BufferSize);

#ifdef RELEASE
	(name);
#else
	Resource->SetName(name.c_str());
#endif

	CreateDerivedViews();
}

void RGpuBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize, const void* initialData /*= nullptr*/)
{
	ElementCount = NumElements;
	ElementSize = ElementSize;
	BufferSize = NumElements * ElementSize;

	D3D12_RESOURCE_DESC ResourceDesc = GetDesc();

	UsageState = D3D12_RESOURCE_STATE_COMMON;

	ASSERTHR(SLLRenderer::GetDevice()->CreatePlacedResource(pBackingHeap, HeapOffset, &ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&Resource)));

	GpuVirtualAddress = Resource->GetGPUVirtualAddress();

	if (initialData)
		RCommandList::UploadToBufferImmediate(*this, initialData, BufferSize);

#ifdef RELEASE
	(name);
#else
	Resource->SetName(name.c_str());
#endif

	CreateDerivedViews();
}

D3D12_CPU_DESCRIPTOR_HANDLE RGpuBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
{
	assert(Offset + Size <= BufferSize);

	Size = RMath::AlignUp(Size, 16);

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GpuVirtualAddress + (size_t)Offset;
	CBVDesc.SizeInBytes = Size;

	D3D12_CPU_DESCRIPTOR_HANDLE hCBV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	SLLRenderer::GetDevice()->CreateConstantBufferView(&CBVDesc, hCBV);
	return hCBV;
}

D3D12_RESOURCE_DESC RGpuBuffer::GetDesc()
{
	assert(BufferSize != 0);

	D3D12_RESOURCE_DESC Desc = {};
	Desc.Alignment = 0;
	Desc.DepthOrArraySize = 1;
	Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags = ResourceFlags;
	Desc.Format = DXGI_FORMAT_UNKNOWN;
	Desc.Height = 1;
	Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels = 1;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Width = (UINT64)BufferSize;
	return Desc;
}

void RReadbackBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize)
{
	Destroy();

	ElementCount = NumElements;
	ElementSize = ElementSize;
	BufferSize = NumElements * ElementSize;
	UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

	// Create a readback buffer large enough to hold all texel data
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	// Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = BufferSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERTHR(SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Resource)));

	GpuVirtualAddress = Resource->GetGPUVirtualAddress();

#ifdef RELEASE
	(name);
#else
	Resource->SetName(name.c_str());
#endif
}

void* RReadbackBuffer::Map(void)
{
	void* Memory;
	auto Range = CD3DX12_RANGE(0, BufferSize);
	Resource->Map(0, &Range, &Memory);
	return Memory;
}

void RReadbackBuffer::Unmap(void)
{
	auto Range = CD3DX12_RANGE(0, 0);
	Resource->Unmap(0, &Range);
}

void RStructuredBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = ElementCount;
	SRVDesc.Buffer.StructureByteStride = ElementSize;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	if (SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		SRV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SLLRenderer::GetDevice()->CreateShaderResourceView(Resource.Get(), &SRVDesc, SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.Buffer.CounterOffsetInBytes = 0;
	UAVDesc.Buffer.NumElements = ElementCount;
	UAVDesc.Buffer.StructureByteStride = ElementSize;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

	if (UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		UAV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SLLRenderer::GetDevice()->CreateUnorderedAccessView(Resource.Get(), CounterBuffer.GetResource(), &UAVDesc, UAV);

}

const D3D12_CPU_DESCRIPTOR_HANDLE& RStructuredBuffer::GetCounterSRV(RCommandList& Context)
{
	Context.TransitionResource(CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	return CounterBuffer.GetSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& RStructuredBuffer::GetCounterUAV(RCommandList& Context)
{
	Context.TransitionResource(CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	return CounterBuffer.GetUAV();
}

void RTypedBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = m_DataFormat;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = ElementCount;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	if (SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		SRV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SLLRenderer::GetDevice()->CreateShaderResourceView(Resource.Get(), &SRVDesc, SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = m_DataFormat;
	UAVDesc.Buffer.NumElements = ElementCount;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	if (UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		UAV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SLLRenderer::GetDevice()->CreateUnorderedAccessView(Resource.Get(), nullptr, &UAVDesc, UAV);
}

void RByteAddressBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = (UINT)BufferSize / 4;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	if (SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		SRV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SLLRenderer::GetDevice()->CreateShaderResourceView(Resource.Get(), &SRVDesc, SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	UAVDesc.Buffer.NumElements = (UINT)BufferSize / 4;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	if (UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		UAV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SLLRenderer::GetDevice()->CreateUnorderedAccessView(Resource.Get(), nullptr, &UAVDesc, UAV);
}

void SCBufferPool::Create(const std::wstring& name, UINT sizeBytes)
{
	Destroy();

	ResetResourceFlags();

	BufferSize = RMath::AlignUpWithMask(sizeBytes, 255);
	BufferSize = BufferSize > kCpuAllocatorPageSize ? BufferSize : kCpuAllocatorPageSize;

	D3D12_RESOURCE_DESC ResourceDesc = GetDesc();

	UsageState = D3D12_RESOURCE_STATE_GENERIC_READ;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	ASSERTHR(
		SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
			&ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&Resource)));

	GpuVirtualAddress = Resource->GetGPUVirtualAddress();

#ifdef RELEASE
	(name);
#else
	Resource->SetName(name.c_str());
#endif
}

SCBufferPool::RCBufferDesc SCBufferPool::CreateCBuffer(UINT size)
{
	size = RMath::AlignUpWithMask(size);

	assert(FreeMemoryByteOffset + size < BufferSize);

	RCBufferDesc newCBuffer;
	newCBuffer.CpuAddress = CpuAddressBase + FreeMemoryByteOffset;
	newCBuffer.GpuAddress = GpuVirtualAddress + FreeMemoryByteOffset;
	newCBuffer.Size = size;

	newCBuffer.Range = CD3DX12_RANGE(FreeMemoryByteOffset, FreeMemoryByteOffset + size );

	newCBuffer.DescHandleView.CPU = CreateConstantBufferView(FreeMemoryByteOffset, size);

	FreeMemoryByteOffset += size;

	return newCBuffer;
}

void SCBufferPool::UpdateCBuffer(RCBufferDesc& cbuffer, void* Data)
{
	CD3DX12_RANGE NullRange{ 0,0 };
	Resource->Map(0, &NullRange, &cbuffer.CpuAddress);
	SIMDMemCopy(cbuffer.CpuAddress, Data, RMath::DivideByMultiple(cbuffer.Size, 16));
	Resource->Unmap(0, &NullRange);
}

void RVertexBuffer::Create(const std::wstring& Name, RVertexBufferCPU& vertexBufferCpu)
{
	RGpuBuffer::Create(Name, vertexBufferCpu.GetNumVertices(), vertexBufferCpu.GetVertexSize(), vertexBufferCpu.Data.data());
}
