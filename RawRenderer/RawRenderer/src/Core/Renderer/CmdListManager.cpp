#include "CmdListManager.h"

#include <algorithm>

#include "LLRenderer.h"


RCommandAllocatorPool::RCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) : CommandListType(Type)
{

}

void RCommandAllocatorPool::Shutdown()
{
	for (size_t i = 0; i < AllocatorPool.size(); i++)
	{
		AllocatorPool[i]->Release();
	}
	AllocatorPool.clear();
}

ID3D12CommandAllocator* RCommandAllocatorPool::RequestAllocator(uint64_t CurrentFenceValue)
{
	std::lock_guard<std::mutex> LockGuard(AllocatorMutex);

	ID3D12CommandAllocator* pAllocator = nullptr;

	if (!AllocatorsReadyToBeReUsed.empty())
	{
		std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = AllocatorsReadyToBeReUsed.front();

		if (AllocatorPair.first < CurrentFenceValue)
		{
			pAllocator = AllocatorPair.second;
			ASSERTHR(pAllocator->Reset());
			AllocatorsReadyToBeReUsed.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (pAllocator == nullptr)
	{
		ASSERTHR(SLLRenderer::GetDevice()->CreateCommandAllocator(CommandListType, IID_PPV_ARGS(&pAllocator)));
		wchar_t AllocatorName[32];
		swprintf(AllocatorName, 32, L"CommandAllocator %zu", AllocatorPool.size());
		pAllocator->SetName(AllocatorName);
		AllocatorPool.push_back(pAllocator);
	}

	return pAllocator;
}

void RCommandAllocatorPool::FreeAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
	std::lock_guard<std::mutex> LockGuard(AllocatorMutex);

	// That fence value indicates we are free to reset the allocator
	AllocatorsReadyToBeReUsed.push(std::make_pair(FenceValue, Allocator));
}

void RCommandQueue::Create()
{
	assert(!IsReady());
	assert(AllocatorPool.Size() == 0);

	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = Type;
	QueueDesc.NodeMask = 1;
	SLLRenderer::GetDevice()->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue));
	switch (Type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		CommandQueue->SetName(L"CommandListManager::CommandQueueDirect");
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		CommandQueue->SetName(L"CommandListManager::CommandQueueCompute");
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		CommandQueue->SetName(L"CommandListManager::CommandQueueCopy");
		break;
	default: break;

	}

	ASSERTHR(SLLRenderer::GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
	Fence->SetName(L"CommandListManager::Fence");
	Fence->Signal((uint64_t)Type << 56);

	FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	assert(FenceEventHandle != NULL);

	AllocatorPool.Create();

	assert(IsReady());
}

void RCommandQueue::Shutdown()
{
	if (CommandQueue == nullptr)
		return;

	AllocatorPool.Shutdown();

	CloseHandle(FenceEventHandle);

	Fence->Release();
	Fence = nullptr;

	CommandQueue->Release();
	CommandQueue = nullptr;
}

uint64_t RCommandQueue::IncrementFence(void)
{
	std::lock_guard<std::mutex> LockGuard(FenceMutex);
	CommandQueue->Signal(Fence, NextFenceValue);
	return NextFenceValue++;
}

bool RCommandQueue::IsFenceComplete(uint64_t FenceValue)
{
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (FenceValue > LastCompletedFenceValue)
	{
		LastCompletedFenceValue = std::max(LastCompletedFenceValue, Fence->GetCompletedValue());
	}

	return FenceValue <= LastCompletedFenceValue;
}

void RCommandQueue::QueueWaitForAnotherFence(uint64_t FenceValue)
{
	RCommandQueue& Producer = SLLRenderer::GetCmdQueueManager().GetQueueBasedOnType((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	CommandQueue->Wait(Producer.Fence, FenceValue);
}

void RCommandQueue::QueueWaitForAnotherQueue(RCommandQueue& Producer)
{
	assert(Producer.NextFenceValue > 0);
	CommandQueue->Wait(Producer.Fence, Producer.NextFenceValue - 1);
}

void RCommandQueue::CPUWaitForFence(uint64_t FenceValue)
{
	if (IsFenceComplete(FenceValue))
		return;

	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		std::lock_guard<std::mutex> LockGuard(EventMutex);

		Fence->SetEventOnCompletion(FenceValue, FenceEventHandle);
		WaitForSingleObject(FenceEventHandle, INFINITE);
		LastCompletedFenceValue = FenceValue;
	}
}

uint64_t RCommandQueue::ExecuteCommandList(ID3D12CommandList* List)
{
	std::lock_guard<std::mutex> LockGuard(FenceMutex);

	ASSERTHR(((ID3D12GraphicsCommandList*)List)->Close());

	// Kickoff the command list
	CommandQueue->ExecuteCommandLists(1, &List);

	// Signal the next fence value (with the GPU)
	CommandQueue->Signal(Fence, NextFenceValue);

	const uint64_t FenceValCopy = NextFenceValue; 
	NextFenceValue++;
	return FenceValCopy;
}

ID3D12CommandAllocator* RCommandQueue::RequestAllocator(void)
{
	uint64_t CurFenceValue = Fence->GetCompletedValue();

	return AllocatorPool.RequestAllocator(CurFenceValue);
}

void SCommandQueueManager::CreateNewCmdListAndAllocator(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** outList, ID3D12CommandAllocator** outAllocator)
{
	assert(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
	switch (Type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT: *outAllocator = GraphicsQueue.RequestAllocator(); break;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE: *outAllocator = ComputeQueue.RequestAllocator(); break;
	case D3D12_COMMAND_LIST_TYPE_COPY: *outAllocator = CopyQueue.RequestAllocator(); break;
	}

	ASSERTHR(SLLRenderer::GetDevice()->CreateCommandList(1, Type, *outAllocator, nullptr, IID_PPV_ARGS(outList)));
	(*outList)->SetName(L"CommandList");
}

void SCommandQueueManager::CPUWaitForFence(uint64_t FenceValueWithQueueType)
{
	RCommandQueue& Producer = SLLRenderer::GetCmdQueueManager().GetQueueBasedOnType((D3D12_COMMAND_LIST_TYPE)(FenceValueWithQueueType >> 56));
	Producer.CPUWaitForFence(FenceValueWithQueueType);
}
