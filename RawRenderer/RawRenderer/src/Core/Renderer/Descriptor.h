#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <queue>

#include "RootSignature.h"

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

class RCommandList;

struct RDescriptorHandle
{
	RDescriptorHandle()
	{
		CPU.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		GPU.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	RDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle)
		: CPU(CpuHandle)
	{
		GPU.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	RDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle)
		: CPU(CpuHandle), GPU(GpuHandle)
	{
	}

	RDescriptorHandle operator+ (INT OffsetScaledByDescriptorSize) const
	{
		RDescriptorHandle ret = *this;
		ret += OffsetScaledByDescriptorSize;
		return ret;
	}

	void operator += (INT OffsetScaledByDescriptorSize)
	{
		if (CPU.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			CPU.ptr += OffsetScaledByDescriptorSize;
		if (GPU.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			GPU.ptr += OffsetScaledByDescriptorSize;
	}

	bool IsNull() const { return CPU.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
	bool IsShaderVisible() const { return GPU.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

	D3D12_CPU_DESCRIPTOR_HANDLE CPU;
	D3D12_GPU_DESCRIPTOR_HANDLE GPU;

};

class RDescriptorHeap
{
public:
	void Create(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount, const std::wstring& DebugName = L"DescriptorHeap", D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	RDescriptorHandle Allocate(UINT count = 1);

	void EmplaceBack(RDescriptorHandle& outHandle)
	{
		outHandle = Allocate();
	}

	RDescriptorHandle GetHandle(UINT index) const;

	ID3D12DescriptorHeap* Get()
	{
		return Heap.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	UINT descriptorSize = 0;
	UINT numDescriptorsInHeap = 0;
	UINT numDescriptorsAllocated = 0;

	RDescriptorHandle firstHandle;
	RDescriptorHandle nextFreeHandle;

};

// This is an unbounded resource descriptor allocator.  It is intended to provide space for CPU-visible resource descriptors
// as resources are created.  For those that need to be made shader-visible, they will need to be copied to a UserDescriptorHeap
// or a DynamicDescriptorHeap.
class SDescriptorAllocatorCPU
{
public:
	SDescriptorAllocatorCPU(D3D12_DESCRIPTOR_HEAP_TYPE Type) 
		: Type(Type) 
	{}

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

	static void DestroyAll(void)
	{
		DescriptorHeapPool.clear();
	}

protected:

	static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

	static constexpr uint32_t NumDescriptorsPerHeap = 256;
	static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorHeapPool;

	ID3D12DescriptorHeap* CurrentHeap{ nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentHandle;

	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	uint32_t DescriptorSize;
	uint32_t RemainingFreeHandles;

	static std::mutex AllocationMutex;
};



// Describes a descriptor table entry:  a region of the handle cache and which handles have been set
struct DescriptorTableCache
{
	DescriptorTableCache() : AssignedHandlesBitMap(0) {}
	uint32_t AssignedHandlesBitMap;
	D3D12_CPU_DESCRIPTOR_HANDLE* TableStart;
	uint32_t TableSize;
};

struct DescriptorHandleCache
{
	DescriptorHandleCache()
	{
		ClearCache();
	}

	void ClearCache()
	{
		RootDescriptorTablesBitMap = 0;
		MaxCachedDescriptors = 0;
	}

	uint32_t ComputeStagedSize();

	void CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, RDescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

	void UnbindAllValid();

	void StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

	void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RRootSignature& RootSig);

	static constexpr uint32_t kMaxNumDescriptors = 256;
	static constexpr uint32_t kMaxNumDescriptorTables = 16;

	DescriptorTableCache RootDescriptorTable[kMaxNumDescriptorTables];
	D3D12_CPU_DESCRIPTOR_HANDLE HandleCache[kMaxNumDescriptors];

	uint32_t RootDescriptorTablesBitMap;
	uint32_t StaleRootParamsBitMap;
	uint32_t MaxCachedDescriptors;

};


// This class is a linear allocation system for dynamically generated descriptor tables. It internally caches
// CPU descriptor handles so that when not enough space is available in the current heap, necessary descriptors
// can be re-copied to the new heap.
class RDynamicDescriptorHeap
{
public:
	RDynamicDescriptorHeap(RCommandList& OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType);

	static void DestroyAll(void)
	{
		DescriptorHeapPool[0].clear();
		DescriptorHeapPool[1].clear();
	}

	void CleanupUsedHeaps(uint64_t fenceValue)
	{
		RetireCurrentHeap();
		RetireUsedHeaps(fenceValue);
		GraphicsHandleCache.ClearCache();
		ComputeHandleCache.ClearCache();
	}

	// Copy multiple handles into the cache area reserved for the specified root parameter.
	void SetGraphicsDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		GraphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
	}

	void SetComputeDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		ComputeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
	}

	// Bypass the cache and upload directly to the shader-visible heap
	D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle);

	// Deduce cache layout needed to support the descriptor tables needed by the root signature.
	void ParseGraphicsRootSignature(const RRootSignature& RootSig)
	{
		GraphicsHandleCache.ParseRootSignature(DescriptorType, RootSig);
	}

	void ParseComputeRootSignature(const RRootSignature& RootSig)
	{
		ComputeHandleCache.ParseRootSignature(DescriptorType, RootSig);
	}

	// Upload any new descriptors in the cache to the shader-visible heap.
	inline void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
	{
		if (GraphicsHandleCache.StaleRootParamsBitMap != 0)
			CopyAndBindStagedTables(GraphicsHandleCache, CmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}

	inline void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
	{
		if (ComputeHandleCache.StaleRootParamsBitMap != 0)
			CopyAndBindStagedTables(ComputeHandleCache, CmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	}

private:

	// Static members
	static constexpr uint32_t kNumDescriptorsPerHeap = 1024;
	static std::mutex Mutex;
	static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorHeapPool[2];
	static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> RetiredDescriptorHeaps[2];
	static std::queue<ID3D12DescriptorHeap*> AvailableDescriptorHeaps[2];

	// Static methods
	static ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
	static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps);

	// Non-static members
	RCommandList& OwningContext;
	ID3D12DescriptorHeap* CurrentHeap = nullptr;
	const D3D12_DESCRIPTOR_HEAP_TYPE DescriptorType;
	uint32_t DescriptorSize;
	uint32_t CurrentOffset = 0;
	RDescriptorHandle FirstDescriptor;
	std::vector<ID3D12DescriptorHeap*> RetiredHeaps;

	DescriptorHandleCache GraphicsHandleCache;
	DescriptorHandleCache ComputeHandleCache;

	bool HasSpace(uint32_t Count)
	{
		return (CurrentHeap != nullptr && CurrentOffset + Count <= kNumDescriptorsPerHeap);
	}

	void RetireCurrentHeap(void);
	void RetireUsedHeaps(uint64_t fenceValue)
	{
		DiscardDescriptorHeaps(DescriptorType, fenceValue, RetiredHeaps);
		RetiredHeaps.clear();
	}
	ID3D12DescriptorHeap* GetHeapPointer()
	{
		if (CurrentHeap == nullptr)
		{
			assert(CurrentOffset == 0);
			CurrentHeap = RequestDescriptorHeap(DescriptorType);
			FirstDescriptor = RDescriptorHandle(
				CurrentHeap->GetCPUDescriptorHandleForHeapStart(),
				CurrentHeap->GetGPUDescriptorHandleForHeapStart());
		}

		return CurrentHeap;
	}

	RDescriptorHandle Allocate(UINT Count)
	{
		RDescriptorHandle ret = FirstDescriptor + CurrentOffset * DescriptorSize;
		CurrentOffset += Count;
		return ret;
	}

	void CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

	// Mark all descriptors in the cache as stale and in need of re-uploading.
	void UnbindAllValid(void)
	{
		GraphicsHandleCache.UnbindAllValid();
		ComputeHandleCache.UnbindAllValid();
	}

};