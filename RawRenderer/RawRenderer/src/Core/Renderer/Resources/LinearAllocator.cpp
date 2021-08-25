#include "LinearAllocator.h"

#include <cassert>
#include "Utilities/Math/Misc.h"
#include "LLRenderer.h"

ELinearAllocatorType SLinearGpuMemoryPageAllocator::AutoType = kGpuExclusive;

SLinearGpuMemoryPageAllocator RLinearGpuMemoryAllocator::PageManager[2];

SLinearGpuMemoryPageAllocator::SLinearGpuMemoryPageAllocator()
{
	AllocationType = AutoType;
	AutoType = (ELinearAllocatorType)(AutoType + 1);
	assert(AutoType <= kNumAllocatorTypes);
}

RLinearGpuMemoryPage* SLinearGpuMemoryPageAllocator::RequestMemoryPage(void)
{
	std::lock_guard<std::mutex> LockGuard(Mutex);

	//Wait untill all freed pages are available
	while (!RetiredPages.empty() && SLLRenderer::GetCmdQueueManager().IsFenceComplete(RetiredPages.front().first))
	{
		AvailablePages.push(RetiredPages.front().second);
		RetiredPages.pop();
	}

	RLinearGpuMemoryPage* PagePtr = nullptr;

	if (!AvailablePages.empty())
	{
		PagePtr = AvailablePages.front();
		AvailablePages.pop();
	}
	else
	{
		PagePtr = CreateNewMemoryPage();
		PagePool.emplace_back(PagePtr);
	}

	return PagePtr;
}

RLinearGpuMemoryPage* SLinearGpuMemoryPageAllocator::CreateNewMemoryPage(size_t PageSize /*= 0*/)
{
	D3D12_HEAP_PROPERTIES HeapProps;
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

	D3D12_RESOURCE_STATES DefaultUsage;

	if (AllocationType == kGpuExclusive)
	{
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		ResourceDesc.Width = PageSize == 0 ? kGpuAllocatorPageSize : PageSize;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		DefaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else //Cpu
	{
		HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		ResourceDesc.Width = PageSize == 0 ? kCpuAllocatorPageSize : PageSize;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		DefaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	ID3D12Resource* pBuffer;
	ASSERTHR(SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
		&ResourceDesc, DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)));

	pBuffer->SetName(L"Linear Allocated Memory Page");

	return new RLinearGpuMemoryPage(pBuffer, DefaultUsage);
}

void SLinearGpuMemoryPageAllocator::FreeMemoryPages(uint64_t FenceValue, const std::vector<RLinearGpuMemoryPage*>& Pages)
{
	std::lock_guard<std::mutex> LockGuard(Mutex);
	for (auto iter = Pages.begin(); iter != Pages.end(); ++iter)
	{
		RetiredPages.push(std::make_pair(FenceValue, *iter));
	}
}

void SLinearGpuMemoryPageAllocator::DeleteMemoryPages(uint64_t FenceValue, const std::vector<RLinearGpuMemoryPage*>& Pages)
{
	std::lock_guard<std::mutex> LockGuard(Mutex);

	//Wait until Fence is reached and delete
	while (!DeletionQueue.empty() && SLLRenderer::GetCmdQueueManager().IsFenceComplete(DeletionQueue.front().first))
	{
		delete DeletionQueue.front().second;
		DeletionQueue.pop();
	}

	for (auto iter = Pages.begin(); iter != Pages.end(); ++iter)
	{
		(*iter)->Unmap();
		DeletionQueue.push(std::make_pair(FenceValue, *iter));
	}
}

RLinearGpuMemoryAllocator::RLinearGpuMemoryAllocator(ELinearAllocatorType Type) : AllocationType(Type)
{
	assert(Type > kInvalidAllocator && Type < kNumAllocatorTypes);
	PageSize = (Type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
}

RLinearGpuMemory RLinearGpuMemoryAllocator::AllocateFromPage(size_t SizeInBytes, size_t Alignment /*= DEFAULT_ALIGN*/)
{
	const size_t AlignmentMask = Alignment - 1;

	// Assert that it's a power of two.
	assert((AlignmentMask & Alignment) == 0);

	// Align the allocation
	const size_t AlignedSize = RMath::AlignUpWithMask(SizeInBytes, AlignmentMask);

	if (AlignedSize > PageSize)
		return AllocateLargePage(AlignedSize);

	CurOffset = RMath::AlignUp(CurOffset, Alignment);

	if (CurOffset + AlignedSize > PageSize)
	{
		assert(CurPage != nullptr);
		RetiredPages.push_back(CurPage);
		CurPage = nullptr;
	}

	if (CurPage == nullptr)
	{
		CurPage = PageManager[AllocationType].RequestMemoryPage();
		CurOffset = 0;
	}

	RLinearGpuMemory ret(*CurPage, CurOffset, AlignedSize);
	ret.DataPtr = (uint8_t*)CurPage->CpuVirtualAddress + CurOffset;
	ret.GpuAddress = CurPage->GpuVirtualAddress + CurOffset;

	CurOffset += AlignedSize;

	return ret;
}

void RLinearGpuMemoryAllocator::CleanupUsedPages(uint64_t FenceValue)
{
	if (CurPage == nullptr)
		return;

	RetiredPages.push_back(CurPage);
	CurPage = nullptr;
	CurOffset = 0;

	PageManager[AllocationType].FreeMemoryPages(FenceValue, RetiredPages);
	RetiredPages.clear();

	PageManager[AllocationType].DeleteMemoryPages(FenceValue, LargePageList);
	LargePageList.clear();
}

RLinearGpuMemory RLinearGpuMemoryAllocator::AllocateLargePage(size_t SizeInBytes)
{
	RLinearGpuMemoryPage* OneOff = PageManager[AllocationType].CreateNewMemoryPage(SizeInBytes);
	LargePageList.push_back(OneOff);

	RLinearGpuMemory ret(*OneOff, 0, SizeInBytes);
	ret.DataPtr = OneOff->CpuVirtualAddress;
	ret.GpuAddress = OneOff->GpuVirtualAddress;

	return ret;
}

