#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl/client.h>

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

class RUploadBufferDynamic
{
public:
	RUploadBufferDynamic() = default;
	~RUploadBufferDynamic() { Destroy(); }

	void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize);
	void Destroy(void)
	{
		if (Resource.Get() != nullptr)
		{
			if (CpuVirtualAddress != nullptr)
				Unmap();

			Resource = nullptr;
			GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
		}
	}

	// Map a CPU-visible pointer to the buffer memory.  You probably don't want to leave a lot of
	// memory (100s of MB) mapped this way, so you have the option of unmapping it.
	void* Map(void);
	void Unmap(void);

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t NumVertices, uint32_t Stride, uint32_t Offset = 0) const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = GpuVirtualAddress + Offset;
		vbv.SizeInBytes = NumVertices * Stride;
		vbv.StrideInBytes = Stride;
		return vbv;
	}
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(uint32_t NumIndices, bool _32bit, uint32_t Offset = 0) const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = GpuVirtualAddress + Offset;
		ibv.Format = _32bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = NumIndices * (_32bit ? 4 : 2);
		return ibv;
	}
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(uint32_t Offset = 0) const
	{
		return GpuVirtualAddress + Offset;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
	void* CpuVirtualAddress = nullptr;
};