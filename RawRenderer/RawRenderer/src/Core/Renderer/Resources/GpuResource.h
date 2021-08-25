#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

class RGpuResource
{
	friend class RCommandList;
	friend class RCommandListGraphics;
	friend class RComputeContext;
	friend class RLinearGpuMemoryAllocator;

public:

	RGpuResource() = default;

	RGpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		Resource(pResource),
		UsageState(CurrentState)
	{
	}

	virtual void Destroy()
	{
		Resource = nullptr;
		GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
		if (UserAllocatedMemory != nullptr)
		{
			VirtualFree(UserAllocatedMemory, 0, MEM_RELEASE);
			UserAllocatedMemory = nullptr;
		}
	}

	ID3D12Resource* GetResource()
	{
		return Resource.Get();
	}

	const ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return GpuVirtualAddress; }

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	D3D12_RESOURCE_STATES UsageState{ D3D12_RESOURCE_STATE_COMMON };
	D3D12_RESOURCE_STATES TransitioningState{ (D3D12_RESOURCE_STATES)-1};
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };

	// When using VirtualAlloc() to allocate memory directly, record the allocation here so that it can be freed.  The
	// GpuVirtualAddress may be offset from the true allocation start.
	void* UserAllocatedMemory{ nullptr };
};
