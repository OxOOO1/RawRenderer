#pragma once

#include <mutex>
#include <vector>
#include <queue>
#include <stdint.h>

#include <d3d12.h>

class RCommandAllocatorPool
{
public:
	RCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
	~RCommandAllocatorPool()
	{
		Shutdown();
	}

	void Create()
	{

	}
	void Shutdown();

	ID3D12CommandAllocator* RequestAllocator(uint64_t CurrentFenceValue);

	void FreeAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

	inline size_t Size() { return AllocatorPool.size(); }

private:
	const D3D12_COMMAND_LIST_TYPE CommandListType;

	std::vector<ID3D12CommandAllocator*> AllocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> AllocatorsReadyToBeReUsed;
	std::mutex AllocatorMutex;
};

class RCommandQueue
{
	friend class SCommandQueueManager;
	friend class RCommandList;

public:
	RCommandQueue(D3D12_COMMAND_LIST_TYPE Type)
		:
		Type(Type),
		NextFenceValue((uint64_t)Type << 56 | 1),
		LastCompletedFenceValue((uint64_t)Type << 56),
		AllocatorPool(Type)
	{

	}

	~RCommandQueue()
	{
		Shutdown();
	}

	void Create();

	void Shutdown();

	inline bool IsReady()
	{
		return CommandQueue != nullptr;
	}

	uint64_t IncrementFence(void);

	bool IsFenceComplete(uint64_t FenceValue);

	void QueueWaitForAnotherFence(uint64_t FenceValue);

	void QueueWaitForAnotherQueue(RCommandQueue& Producer);

	void CPUWaitForFence(uint64_t FenceValue);

	void CPUWaitForQueue(void)
	{ 
		auto SignalValue = IncrementFence();
		CPUWaitForFence(SignalValue);
	}

	ID3D12CommandQueue* GetCommandQueue() { return CommandQueue; }

	uint64_t GetNextFenceValue() { return NextFenceValue; }

private:

	uint64_t ExecuteCommandList(ID3D12CommandList* List);

	ID3D12CommandAllocator* RequestAllocator(void);

	void FreeAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator)
	{
		AllocatorPool.FreeAllocator(FenceValueForReset, Allocator);
	}

	ID3D12CommandQueue* CommandQueue = nullptr;

	const D3D12_COMMAND_LIST_TYPE Type;

	RCommandAllocatorPool AllocatorPool;
	std::mutex FenceMutex;
	std::mutex EventMutex;

	// Lifetime of these objects is managed by the descriptor cache
	ID3D12Fence* Fence = nullptr;
	uint64_t NextFenceValue;
	uint64_t LastCompletedFenceValue;
	HANDLE FenceEventHandle;
};


class SCommandQueueManager
{

public:
	SCommandQueueManager()
		:
		GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
		ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
		CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
	{
		
	}

	~SCommandQueueManager()
	{
		Shutdown();
	}

	void Create()
	{
		GraphicsQueue.Create();
		ComputeQueue.Create();
		CopyQueue.Create();
	}
	void Shutdown()
	{
		GraphicsQueue.Shutdown();
		ComputeQueue.Shutdown();
		CopyQueue.Shutdown();
	}

	RCommandQueue& GetGraphicsQueue(void) { return GraphicsQueue; }
	RCommandQueue& GetComputeQueue(void) { return ComputeQueue; }
	RCommandQueue& GetCopyQueue(void) { return CopyQueue; }

	RCommandQueue& GetQueueBasedOnType(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
		switch (Type)
		{
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: return ComputeQueue;
		case D3D12_COMMAND_LIST_TYPE_COPY: return CopyQueue;
		default: return GraphicsQueue;
		}
	}

	ID3D12CommandQueue* GetMainCommandQueue()
	{
		return GraphicsQueue.GetCommandQueue();
	}

	void CreateNewCmdListAndAllocator(
		D3D12_COMMAND_LIST_TYPE Type,
		ID3D12GraphicsCommandList** outList,
		ID3D12CommandAllocator** outAllocator);

	// Test to see if a fence has already been reached
	bool IsFenceComplete(uint64_t FenceValueWithQueueType)
	{
		return GetQueueBasedOnType(D3D12_COMMAND_LIST_TYPE(FenceValueWithQueueType >> 56)).IsFenceComplete(FenceValueWithQueueType);
	}

	// The CPU will wait for a fence to reach a specified value
	static void CPUWaitForFence(uint64_t FenceValueWithQueueType);

	// The CPU will wait for all command queues to empty (so that the GPU is idle)
	void CPUWaitForQueues(void)
	{
		GraphicsQueue.CPUWaitForQueue();
		ComputeQueue.CPUWaitForQueue();
		CopyQueue.CPUWaitForQueue();
	}

private:
	RCommandQueue GraphicsQueue;
	RCommandQueue ComputeQueue;
	RCommandQueue CopyQueue;
};