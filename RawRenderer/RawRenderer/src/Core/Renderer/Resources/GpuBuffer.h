#pragma once

#include <d3d12.h>
#include <stdint.h>
#include <string>

#include "GpuResource.h"
#include "Utilities/Math/Misc.h"
#include "Descriptor.h"


#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

class RGpuBuffer : public RGpuResource
{
public:
	virtual ~RGpuBuffer() { Destroy(); }

	// Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command context.
	void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData = nullptr);

	// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
	void CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData = nullptr);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return UAV; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return SRV; }

	D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView(void) const { return GpuVirtualAddress; }

	D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

	size_t GetBufferSize() const { return BufferSize; }
	uint32_t GetElementCount() const { return ElementCount; }
	uint32_t GetElementSize() const { return ElementSize; }

	void ResetResourceFlags()
	{
		ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
	}

protected:

	RGpuBuffer()
	{
		UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	D3D12_RESOURCE_DESC GetDesc();

	virtual void CreateDerivedViews() = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE SRV;

	size_t BufferSize = 0;
	uint32_t ElementCount = 0;
	uint32_t ElementSize = 0;
	D3D12_RESOURCE_FLAGS ResourceFlags{ D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS };
};

class RVertexBuffer : RGpuBuffer
{
public:
	void Create(const std::wstring& Name, class RVertexBufferCPU& vertexBufferCpu);

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(size_t BaseVertexIndex = 0) const
	{
		size_t Offset = BaseVertexIndex * ElementSize;
		return GetVertexBufferView(Offset, (uint32_t)(BufferSize - Offset), ElementSize);
	}
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
	{
		D3D12_VERTEX_BUFFER_VIEW VBView;
		VBView.BufferLocation = GpuVirtualAddress + Offset;
		VBView.SizeInBytes = Size;
		VBView.StrideInBytes = Stride;
		return VBView;
	}
protected:
	void CreateDerivedViews() override {}
};

class RIndexBuffer : public RGpuBuffer
{
public:

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const
	{
		D3D12_INDEX_BUFFER_VIEW IBView;
		IBView.BufferLocation = GpuVirtualAddress + Offset;
		IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		IBView.SizeInBytes = Size;
		return IBView;
	}
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(size_t StartIndex = 0) const
	{
		size_t Offset = StartIndex * ElementSize;
		return GetIndexBufferView(Offset, (uint32_t)(BufferSize - Offset), ElementSize == 4);
	}
protected:
	void CreateDerivedViews() override {}
};

class SCBufferPool : public RGpuBuffer
{
public:
	
	void Create(const std::wstring& name, UINT size);

	struct RCBufferDesc
	{
		RDescriptorHandle DescHandleView;
		void* CpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
		UINT Size;
		CD3DX12_RANGE Range;
	};

	RCBufferDesc CreateCBuffer(UINT size);

	void UpdateCBuffer(RCBufferDesc& cbuffer, void* Data);

protected:
	void CreateDerivedViews() override {}

private:
	char* CpuAddressBase;
	UINT FreeMemoryByteOffset = 0;
};

class RByteAddressBuffer : public RGpuBuffer
{
public:
	virtual void CreateDerivedViews(void) override;
};

class RStructuredBuffer : public RGpuBuffer
{
public:
	virtual void Destroy(void) override
	{
		CounterBuffer.Destroy();
		RGpuBuffer::Destroy();
	}

	virtual void CreateDerivedViews(void) override;

	RByteAddressBuffer& GetCounterBuffer(void) { return CounterBuffer; }

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV(RCommandList& Context);
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV(RCommandList& Context);

private:
	RByteAddressBuffer CounterBuffer;
};

class RTypedBuffer : public RGpuBuffer
{
public:
	RTypedBuffer(DXGI_FORMAT Format) : m_DataFormat(Format) {}
	virtual void CreateDerivedViews(void) override;

protected:
	DXGI_FORMAT m_DataFormat;
};

class RIndirectArgsBuffer : public RByteAddressBuffer
{
public:
	RIndirectArgsBuffer(void)
	{
	}
};

class RReadbackBuffer : public RGpuBuffer
{
public:
	virtual ~RReadbackBuffer() { Destroy(); }

	void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize);

	void* Map(void);
	void Unmap(void);

protected:

	void CreateDerivedViews(void) {}

};
