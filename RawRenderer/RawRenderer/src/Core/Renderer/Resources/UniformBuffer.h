#pragma once

#include "LinearAllocator.h"
#include "CommandContext.h"

class RUniformBuffer : RGpuResource
{

public:
	virtual ~RUniformBuffer() { Destroy(); }

	void Create(UINT size);

	template <typename T>
	void UploadToGPU(T& data)
	{
		RCommandList::UploadToBufferImmediate(*this, &data, sizeof(T));
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetView()
	{
		return ViewHandleCPU;
	}

	UINT Size;
	D3D12_CPU_DESCRIPTOR_HANDLE ViewHandleCPU;

};