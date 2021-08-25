#include "Descriptor.h"

#include "LLRenderer.h"

std::mutex RDynamicDescriptorHeap::Mutex;

std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> RDynamicDescriptorHeap::RetiredDescriptorHeaps[2];

std::queue<ID3D12DescriptorHeap*> RDynamicDescriptorHeap::AvailableDescriptorHeaps[2];

std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> RDynamicDescriptorHeap::DescriptorHeapPool[2];

std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> SDescriptorAllocatorCPU::DescriptorHeapPool;

std::mutex SDescriptorAllocatorCPU::AllocationMutex;

void RDescriptorHeap::Create(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount, const std::wstring& DebugName /*= L"DescriptorHeap"*/, D3D12_DESCRIPTOR_HEAP_FLAGS flags /*= D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
{
	Desc.Type = Type;
	Desc.NumDescriptors = MaxCount;
	Desc.Flags = flags;
	Desc.NodeMask = 1u;

	assert(descriptorSize == 0);
	ASSERTHR(SLLRenderer::GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap)));
	descriptorSize = SLLRenderer::GetDevice()->GetDescriptorHandleIncrementSize(Desc.Type);

#ifdef RELEASE
	(void)DebugHeapName;
#else
	Heap->SetName(DebugName.c_str());
#endif

	numDescriptorsAllocated = 0;
	numDescriptorsInHeap = Desc.NumDescriptors;
	firstHandle = RDescriptorHandle(Heap->GetCPUDescriptorHandleForHeapStart(), Heap->GetGPUDescriptorHandleForHeapStart());
	nextFreeHandle = firstHandle;
}

RDescriptorHandle RDescriptorHeap::Allocate(UINT count /*= 1*/)
{
	assert(count <= numDescriptorsInHeap);
	RDescriptorHandle ret = nextFreeHandle;
	nextFreeHandle += count * descriptorSize;
	numDescriptorsAllocated += count;
	return ret;
}

RDescriptorHandle RDescriptorHeap::GetHandle(UINT index) const
{
	assert(index < numDescriptorsAllocated);
	return {
		D3D12_CPU_DESCRIPTOR_HANDLE{firstHandle.CPU.ptr + index * descriptorSize},
		D3D12_GPU_DESCRIPTOR_HANDLE{firstHandle.GPU.ptr + index * descriptorSize}
	};
}

D3D12_CPU_DESCRIPTOR_HANDLE SDescriptorAllocatorCPU::Allocate(uint32_t Count)
{
	if (CurrentHeap == nullptr || RemainingFreeHandles < Count)
	{
		CurrentHeap = RequestNewHeap(Type);
		CurrentHandle = CurrentHeap->GetCPUDescriptorHandleForHeapStart();
		RemainingFreeHandles = NumDescriptorsPerHeap;

		if (DescriptorSize == 0)
			DescriptorSize = SLLRenderer::GetDevice()->GetDescriptorHandleIncrementSize(Type);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ret = CurrentHandle;
	CurrentHandle.ptr += Count * DescriptorSize;
	RemainingFreeHandles -= Count;
	return ret;
}

ID3D12DescriptorHeap* SDescriptorAllocatorCPU::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	std::lock_guard<std::mutex> LockGuard(AllocationMutex);

	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptorsPerHeap;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.NodeMask = 1;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
	ASSERTHR(SLLRenderer::GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
	DescriptorHeapPool.emplace_back(pHeap);
	return pHeap.Get();
}


uint32_t DescriptorHandleCache::ComputeStagedSize()
{
	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t NeededSpace = 0;
	uint32_t RootIndex;
	uint32_t StaleParams = StaleRootParamsBitMap;
	while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
	{
		StaleParams ^= (1 << RootIndex);

		uint32_t MaxSetHandle;
		assert(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
			"Root entry marked as stale but has no stale descriptors");

		NeededSpace += MaxSetHandle + 1;
	}
	return NeededSpace;
}

void DescriptorHandleCache::CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, RDescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList, void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t StaleParamCount = 0;
	uint32_t TableSize[DescriptorHandleCache::kMaxNumDescriptorTables];
	uint32_t RootIndices[DescriptorHandleCache::kMaxNumDescriptorTables];
	uint32_t NeededSpace = 0;
	uint32_t RootIndex;

	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t StaleParams = StaleRootParamsBitMap;
	while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
	{
		RootIndices[StaleParamCount] = RootIndex;
		StaleParams ^= (1 << RootIndex);

		uint32_t MaxSetHandle;
		assert(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
			"Root entry marked as stale but has no stale descriptors");

		NeededSpace += MaxSetHandle + 1;
		TableSize[StaleParamCount] = MaxSetHandle + 1;

		++StaleParamCount;
	}

	assert(StaleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables,
		"We're only equipped to handle so many descriptor tables");

	StaleRootParamsBitMap = 0;

	static const uint32_t kMaxDescriptorsPerCopy = 16;
	UINT NumDestDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
	UINT pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

	UINT NumSrcDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
	UINT pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

	for (uint32_t i = 0; i < StaleParamCount; ++i)
	{
		RootIndex = RootIndices[i];
		(CmdList->*SetFunc)(RootIndex, DestHandleStart.GPU);

		DescriptorTableCache& RootDescTable = RootDescriptorTable[RootIndex];

		D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.TableStart;
		uint64_t SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
		D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart.CPU;
		DestHandleStart += TableSize[i] * DescriptorSize;

		unsigned long SkipCount;
		while (_BitScanForward64(&SkipCount, SetHandles))
		{
			// Skip over unset descriptor handles
			SetHandles >>= SkipCount;
			SrcHandles += SkipCount;
			CurDest.ptr += SkipCount * DescriptorSize;

			unsigned long DescriptorCount;
			_BitScanForward64(&DescriptorCount, ~SetHandles);
			SetHandles >>= DescriptorCount;

			// If we run out of temp room, copy what we've got so far
			if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy)
			{
				SLLRenderer::GetDevice()->CopyDescriptors(
					NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
					NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
					Type);

				NumSrcDescriptorRanges = 0;
				NumDestDescriptorRanges = 0;
			}

			// Setup destination range
			pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
			pDestDescriptorRangeSizes[NumDestDescriptorRanges] = DescriptorCount;
			++NumDestDescriptorRanges;

			// Setup source ranges (one descriptor each because we don't assume they are contiguous)
			for (uint32_t j = 0; j < DescriptorCount; ++j)
			{
				pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
				pSrcDescriptorRangeSizes[NumSrcDescriptorRanges] = 1;
				++NumSrcDescriptorRanges;
			}

			// Move the destination pointer forward by the number of descriptors we will copy
			SrcHandles += DescriptorCount;
			CurDest.ptr += DescriptorCount * DescriptorSize;
		}
	}

	SLLRenderer::GetDevice()->CopyDescriptors(
		NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
		NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
		Type);
}

void DescriptorHandleCache::UnbindAllValid()
{
	StaleRootParamsBitMap = 0;

	unsigned long TableParams = RootDescriptorTablesBitMap;
	unsigned long RootIndex;
	while (_BitScanForward(&RootIndex, TableParams))
	{
		TableParams ^= (1 << RootIndex);
		if (RootDescriptorTable[RootIndex].AssignedHandlesBitMap != 0)
			StaleRootParamsBitMap |= (1 << RootIndex);
	}
}

void DescriptorHandleCache::StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	assert(((1 << RootIndex) & RootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
	assert(Offset + NumHandles <= RootDescriptorTable[RootIndex].TableSize);

	DescriptorTableCache& TableCache = RootDescriptorTable[RootIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = TableCache.TableStart + Offset;
	for (UINT i = 0; i < NumHandles; ++i)
		CopyDest[i] = Handles[i];
	TableCache.AssignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
	StaleRootParamsBitMap |= (1 << RootIndex);
}

void DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RRootSignature& RootSig)
{
	UINT CurrentOffset = 0;

	assert(RootSig.NumParameters <= 16, "Maybe we need to support something greater");

	StaleRootParamsBitMap = 0;
	RootDescriptorTablesBitMap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
		RootSig.SamplerTableBitMap : RootSig.DescriptorTableBitMap);

	unsigned long TableParams = RootDescriptorTablesBitMap;
	unsigned long RootIndex;
	while (_BitScanForward(&RootIndex, TableParams))
	{
		TableParams ^= (1 << RootIndex);

		UINT TableSize = RootSig.DescriptorTableSize[RootIndex];
		assert(TableSize > 0);

		DescriptorTableCache& RootDescriptorTableT = RootDescriptorTable[RootIndex];
		RootDescriptorTableT.AssignedHandlesBitMap = 0;
		RootDescriptorTableT.TableStart = HandleCache + CurrentOffset;
		RootDescriptorTableT.TableSize = TableSize;

		CurrentOffset += TableSize;
	}

	MaxCachedDescriptors = CurrentOffset;

	assert(MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
}

RDynamicDescriptorHeap::RDynamicDescriptorHeap(RCommandList& OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType) :OwningContext(OwningContext), DescriptorType(HeapType)
{
	DescriptorSize = SLLRenderer::GetDevice()->GetDescriptorHandleIncrementSize(HeapType);
}

D3D12_GPU_DESCRIPTOR_HANDLE RDynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	if (!HasSpace(1))
	{
		RetireCurrentHeap();
		UnbindAllValid();
	}

	OwningContext.SetDescriptorHeap(DescriptorType, GetHeapPointer());

	RDescriptorHandle DestHandle = FirstDescriptor + CurrentOffset * DescriptorSize;
	CurrentOffset += 1;

	SLLRenderer::GetDevice()->CopyDescriptorsSimple(1, DestHandle.CPU, Handle, DescriptorType);

	return DestHandle.GPU;
}


ID3D12DescriptorHeap* RDynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
{
	std::lock_guard<std::mutex> LockGuard(Mutex);

	uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

	while (!RetiredDescriptorHeaps[idx].empty() && SLLRenderer::GetCmdQueueManager().IsFenceComplete(RetiredDescriptorHeaps[idx].front().first))
	{
		AvailableDescriptorHeaps[idx].push(RetiredDescriptorHeaps[idx].front().second);
		RetiredDescriptorHeaps[idx].pop();
	}

	if (!AvailableDescriptorHeaps[idx].empty())
	{
		ID3D12DescriptorHeap* HeapPtr = AvailableDescriptorHeaps[idx].front();
		AvailableDescriptorHeaps[idx].pop();
		return HeapPtr;
	}
	else
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.Type = HeapType;
		HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HeapDesc.NodeMask = 1;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;
		ASSERTHR(SLLRenderer::GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));
		DescriptorHeapPool[idx].emplace_back(HeapPtr);
		return HeapPtr.Get();
	}
}

void RDynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps)
{
	uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
	std::lock_guard<std::mutex> LockGuard(Mutex);
	for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
		RetiredDescriptorHeaps[idx].push(std::make_pair(FenceValueForReset, *iter));
}

void RDynamicDescriptorHeap::RetireCurrentHeap(void)
{
	// Don't retire unused heaps.
	if (CurrentOffset == 0)
	{
		assert(CurrentHeap == nullptr);
		return;
	}

	assert(CurrentHeap != nullptr);
	RetiredHeaps.push_back(CurrentHeap);
	CurrentHeap = nullptr;
	CurrentOffset = 0;
}

void RDynamicDescriptorHeap::CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList, void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t NeededSize = HandleCache.ComputeStagedSize();
	if (!HasSpace(NeededSize))
	{
		RetireCurrentHeap();
		UnbindAllValid();
		NeededSize = HandleCache.ComputeStagedSize();
	}

	// This can trigger the creation of a new heap
	OwningContext.SetDescriptorHeap(DescriptorType, GetHeapPointer());
	HandleCache.CopyAndBindStaleTables(DescriptorType, DescriptorSize, Allocate(NeededSize), CmdList, SetFunc);
}
