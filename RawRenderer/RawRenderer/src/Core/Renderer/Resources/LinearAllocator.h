#pragma once

// Description:  This is a dynamic graphics memory allocator for DX12.  It's designed to work in concert
// with the CommandContext class and to do so in a thread-safe manner.  There may be many command contexts,
// each with its own linear allocators.  They act as windows into a global memory pool by reserving a
// context-local memory page.  Requesting a new page is done in a thread-safe manner by guarding accesses
// with a mutex lock.
//
// When a command context is finished, it will receive a fence ID that indicates when it's safe to reclaim
// used resources.  The CleanupUsedPages() method must be invoked at this time so that the used pages can be
// scheduled for reuse after the fence has cleared.

#include "GpuResource.h"
#include <vector>
#include <queue>
#include <mutex>

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256

enum ELinearAllocatorType
{
	kInvalidAllocator = -1,

	kGpuExclusive = 0,        // DEFAULT   GPU-writeable (via UAV)
	kCpuWritable = 1,        // UPLOAD CPU-writeable (but write combined)

	kNumAllocatorTypes
};

enum
{
	kGpuAllocatorPageSize = 0x10000,    // 64K
	kCpuAllocatorPageSize = 0x200000    // 2MB
};

//Upload buffer
class RLinearGpuMemoryPage : public RGpuResource
{
public:
	RLinearGpuMemoryPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Usage) : RGpuResource()
	{
		Resource.Attach(pResource);
		UsageState = Usage;
		GpuVirtualAddress = Resource->GetGPUVirtualAddress();
		Resource->Map(0, nullptr, &CpuVirtualAddress);
	}

	~RLinearGpuMemoryPage()
	{
		Unmap();
	}

	void Map(void)
	{
		if (CpuVirtualAddress == nullptr)
		{
			Resource->Map(0, nullptr, &CpuVirtualAddress);
		}
	}

	void Unmap(void)
	{
		if (CpuVirtualAddress != nullptr)
		{
			Resource->Unmap(0, nullptr);
			CpuVirtualAddress = nullptr;
		}
	}

	void* CpuVirtualAddress;
	//D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
};

//Allocated from UploadBuffer page 
struct RLinearGpuMemory
{
	RLinearGpuMemory(RGpuResource& BaseResource, size_t ThisOffset, size_t ThisSize)
		: Buffer(BaseResource), Offset(ThisOffset), Size(ThisSize) {}

	RGpuResource& Buffer;    // The D3D buffer associated with this memory.
	size_t Offset;            // Offset from start of buffer resource
	size_t Size;            // Reserved size of this allocation
	void* DataPtr;            // The CPU-writeable address
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;    // The GPU-visible address
};

class SLinearGpuMemoryPageAllocator
{
public:

	SLinearGpuMemoryPageAllocator();

	void Destroy(void) { PagePool.clear(); }

	RLinearGpuMemoryPage* RequestMemoryPage(void);

	RLinearGpuMemoryPage* CreateNewMemoryPage(size_t PageSize = 0);

	// Discarded pages will get recycled.  This is for fixed size pages.
	void FreeMemoryPages(uint64_t FenceValue, const std::vector<RLinearGpuMemoryPage*>& Pages);

	// Freed pages will be destroyed once their fence has passed.  This is for single-use,
	// "large" pages.
	void DeleteMemoryPages(uint64_t FenceValue, const std::vector<RLinearGpuMemoryPage*>& Pages);

private:
	static ELinearAllocatorType AutoType;

	ELinearAllocatorType AllocationType;
	//All pages
	std::vector<std::unique_ptr<RLinearGpuMemoryPage> > PagePool;
	//Buffers that can be recycled
	std::queue<std::pair<uint64_t, RLinearGpuMemoryPage*> > RetiredPages;
	//Buffers submitted for deletion when no longer needed
	std::queue<std::pair<uint64_t, RLinearGpuMemoryPage*> > DeletionQueue;
	//Retired pages that can be re-used
	std::queue<RLinearGpuMemoryPage*> AvailablePages;
	std::mutex Mutex;
};

class RLinearGpuMemoryAllocator
{
public:

	RLinearGpuMemoryAllocator(ELinearAllocatorType Type);

	RLinearGpuMemory AllocateFromPage(size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN);

	void CleanupUsedPages(uint64_t FenceValue);

	static void DestroyAll(void)
	{
		PageManager[0].Destroy();
		PageManager[1].Destroy();
	}

private:

	RLinearGpuMemory AllocateLargePage(size_t SizeInBytes);

	static SLinearGpuMemoryPageAllocator PageManager[2];

	ELinearAllocatorType AllocationType;
	size_t PageSize = 0;
	size_t CurOffset = ~(size_t)0;

	RLinearGpuMemoryPage* CurPage = nullptr;
	std::vector<RLinearGpuMemoryPage*> RetiredPages;
	std::vector<RLinearGpuMemoryPage*> LargePageList;

};