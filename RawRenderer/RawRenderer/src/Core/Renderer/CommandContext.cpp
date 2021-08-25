#include "CommandContext.h"

#include "LLRenderer.h"
#include "Descriptor.h"

#include "Utilities/Profiling.h"

RCommandList* SCommandListManager::AllocateCmdList(D3D12_COMMAND_LIST_TYPE Type)
{
	std::lock_guard<std::mutex> LockGuard(ContextAllocationMutex);

	auto& AvailableContexts = AvailableContextsArr[Type];

	RCommandList* pCmdConext = nullptr;
	if (AvailableContexts.empty())
	{
		pCmdConext = new RCommandList(Type);
		//pCmdConext->Initialize();
		ContextPoolArr[Type].emplace_back(pCmdConext);
	}
	else
	{
		pCmdConext = AvailableContexts.front();
		pCmdConext->Reset();
		AvailableContexts.pop();
	}
	assert(pCmdConext != nullptr);

	assert(pCmdConext->Type == Type);

	return pCmdConext;
}

void SCommandListManager::FreeContext(RCommandList* UsedContext)
{
	assert(UsedContext != nullptr);
	std::lock_guard<std::mutex> LockGuard(ContextAllocationMutex);
	AvailableContextsArr[UsedContext->Type].push(UsedContext);
}

RCommandList::RCommandList(D3D12_COMMAND_LIST_TYPE Type) :
	Type(Type),
	DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	MemoryAllocatorLinearUpload(kCpuWritable),
	MemoryAllocatorLinearDefault(kGpuExclusive)
{
	ZeroMemory(CurrentDescriptorHeaps, sizeof(CurrentDescriptorHeaps));
	Initialize();
}

void RCommandList::Reset()
{
	// We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	assert(CmdList != nullptr && CurCmdAllocator == nullptr);
	CurCmdAllocator = SLLRenderer::GetCmdQueueManager().GetQueueBasedOnType(Type).RequestAllocator();
	CmdList->Reset(CurCmdAllocator, nullptr);

	CurGraphicsRootSignature = nullptr;
	CurPipelineState = nullptr;
	CurComputeRootSignature = nullptr;
	NumBarriersToFlush = 0;

	BindDescriptorHeaps();
}

void RCommandList::DestroyAllContexts(void)
{
	RLinearGpuMemoryAllocator::DestroyAll();
	RDynamicDescriptorHeap::DestroyAll();
	SLLRenderer::GetCmdListManager().DestroyAllContexts();
}

RCommandList& RCommandList::BeginNew(const std::wstring ID /*= L""*/)
{
	RCommandList* NewContext = SLLRenderer::GetCmdListManager().AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	NewContext->SetID(ID);
	if (ID.length() > 0)
	{
		RProfileEvent::BeginNew(std::string{ID.begin(), ID.end()}, NewContext);
	}
	return *NewContext;
}

uint64_t RCommandList::ExecuteCmdList(bool WaitForCompletion /*= false*/)
{
	FlushResourceBarriers();

	assert(CurCmdAllocator != nullptr);

	auto& CmdQueue = SLLRenderer::GetCmdQueueManager().GetQueueBasedOnType(Type);
	uint64_t FenceValue = CmdQueue.ExecuteCommandList(CmdList);

	if (WaitForCompletion)
		SLLRenderer::GetCmdQueueManager().CPUWaitForFence(FenceValue);
		//CmdQueue.CPUWaitForFence(FenceValue);

	//
	// Reset the command list and restore previous state
	//

	CmdList->Reset(CurCmdAllocator, nullptr);

	if (CurGraphicsRootSignature)
	{
		CmdList->SetGraphicsRootSignature(CurGraphicsRootSignature);
	}
	if (CurComputeRootSignature)
	{
		CmdList->SetComputeRootSignature(CurComputeRootSignature);
	}
	if (CurPipelineState)
	{
		CmdList->SetPipelineState(CurPipelineState);
	}

	BindDescriptorHeaps();

	return FenceValue;
}

uint64_t RCommandList::ExecuteCmdListAndReleaseContext(bool WaitForCompletion /*= false*/)
{
	assert(Type == D3D12_COMMAND_LIST_TYPE_DIRECT || Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

	FlushResourceBarriers();

	if (IDString.length() > 0)
	{
		RProfileEvent::EndCurrent();
	}

	assert(CurCmdAllocator != nullptr);

	RCommandQueue& Queue = SLLRenderer::GetCmdQueueManager().GetQueueBasedOnType(Type);

	uint64_t FenceValue = Queue.ExecuteCommandList(CmdList);
	Queue.FreeAllocator(FenceValue, CurCmdAllocator);
	CurCmdAllocator = nullptr;

	MemoryAllocatorLinearUpload.CleanupUsedPages(FenceValue);
	MemoryAllocatorLinearDefault.CleanupUsedPages(FenceValue);
	DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
	DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

	if (WaitForCompletion)
		SLLRenderer::GetCmdQueueManager().CPUWaitForFence(FenceValue);

	SLLRenderer::GetCmdListManager().FreeContext(this);

	return FenceValue;
}

void RCommandList::Initialize(void)
{
	SLLRenderer::GetCmdQueueManager().CreateNewCmdListAndAllocator(Type, &CmdList, &CurCmdAllocator);
}

void RCommandList::CopyBuffer(RGpuResource& Dest, RGpuResource& Src)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();
	CmdList->CopyResource(Dest.GetResource(), Src.GetResource());
}

void RCommandList::CopyBufferRegion(RGpuResource& Dest, size_t DestOffset, RGpuResource& Src, size_t SrcOffset, size_t NumBytes)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	//TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();
	CmdList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
}

void RCommandList::CopySubresource(RGpuResource& Dest, UINT DestSubIndex, RGpuResource& Src, UINT SrcSubIndex)
{
	FlushResourceBarriers();

	D3D12_TEXTURE_COPY_LOCATION DestLocation =
	{
		Dest.GetResource(),
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		DestSubIndex
	};

	D3D12_TEXTURE_COPY_LOCATION SrcLocation =
	{
		Src.GetResource(),
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		SrcSubIndex
	};

	CmdList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

void RCommandList::CopyCounter(RGpuResource& Dest, size_t DestOffset, RStructuredBuffer& Src)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();
	CmdList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
}

void RCommandList::ResetCounter(RStructuredBuffer& Buf, uint32_t Value /*= 0*/)
{
	FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
	TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void RCommandList::UploadToTextureImmediate(RGpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

	RCommandList& ImmediateContext = RCommandList::BeginNew(L"UploadToTextureImmediate");

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	RLinearGpuMemory mem = ImmediateContext.ReserveUploadMemory(uploadBufferSize);
	UpdateSubresources(ImmediateContext.CmdList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0, NumSubresources, SubData);
	ImmediateContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	ImmediateContext.ExecuteCmdListAndReleaseContext(true);
}

void RCommandList::UploadToBufferImmediate(RGpuResource& Dest, const void* Data, size_t NumBytes, size_t Offset /*= 0*/)
{
	RCommandList& InitContext = RCommandList::BeginNew(L"UploadToBufferImmediate");

	RLinearGpuMemory mem = InitContext.ReserveUploadMemory(NumBytes);
	if (RMath::IsAligned(mem.DataPtr, 16) && RMath::IsAligned(Data, 16))
	{
		SIMDMemCopy(mem.DataPtr, Data, RMath::DivideByMultiple(NumBytes, 16));
	}
	else
	{
		std::memcpy(mem.DataPtr, Data, NumBytes);
	}

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	InitContext.CmdList->CopyBufferRegion(Dest.GetResource(), Offset, mem.Buffer.GetResource(), 0, NumBytes);
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.ExecuteCmdListAndReleaseContext(true);
}

void RCommandList::InitializeTextureArraySliceImmediate(RGpuResource& Dest, UINT SliceIndex, RGpuResource& Src)
{
	RCommandList& Context = RCommandList::BeginNew();

	Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	Context.FlushResourceBarriers();

	const D3D12_RESOURCE_DESC& DestDesc = Dest.GetResource()->GetDesc();
	const D3D12_RESOURCE_DESC& SrcDesc = Src.GetResource()->GetDesc();

	assert(SliceIndex < DestDesc.DepthOrArraySize&&
		SrcDesc.DepthOrArraySize == 1 &&
		DestDesc.Width == SrcDesc.Width &&
		DestDesc.Height == SrcDesc.Height &&
		DestDesc.MipLevels <= SrcDesc.MipLevels
	);

	UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

	for (UINT i = 0; i < DestDesc.MipLevels; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
		{
			Dest.GetResource(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			SubResourceIndex + i
		};

		D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
		{
			Src.GetResource(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			i
		};

		Context.CmdList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
	}

	Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
	Context.ExecuteCmdListAndReleaseContext(true);
}

void RCommandList::ReadbackTexture2DImmediate(RGpuResource& ReadbackBuffer, RPixelBuffer& SrcBuffer)
{
	// The footprint may depend on the device of the resource, but we assume there is only one device.
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
	auto SrcDesc = SrcBuffer.GetResource()->GetDesc();
	SLLRenderer::GetDevice()->GetCopyableFootprints(&SrcDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

	// This very short command list only issues one API call and will be synchronized so we can immediately read
	// the buffer contents.
	RCommandList& ImmediateContext = RCommandList::BeginNew(L"Copy texture to memory");

	ImmediateContext.TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

	auto DestLocation = CD3DX12_TEXTURE_COPY_LOCATION(ReadbackBuffer.GetResource(), PlacedFootprint);
	auto SrcLocation = CD3DX12_TEXTURE_COPY_LOCATION(SrcBuffer.GetResource(), 0);
	ImmediateContext.CmdList->CopyTextureRegion(
		&DestLocation, 0, 0, 0,
		&SrcLocation, nullptr);

	ImmediateContext.ExecuteCmdListAndReleaseContext(true);
}

void RCommandList::WriteBuffer(RGpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes)
{
	assert(Data != nullptr && RMath::IsAligned(Data, 16));
	RLinearGpuMemory TempSpace = MemoryAllocatorLinearUpload.AllocateFromPage(NumBytes, 512);
	SIMDMemCopy(TempSpace.DataPtr, Data, RMath::DivideByMultiple(NumBytes, 16));
	CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void RCommandList::FillBuffer(RGpuResource& Dest, size_t DestOffset, RTypesUnion Value, size_t NumBytes)
{
	RLinearGpuMemory TempSpace = MemoryAllocatorLinearUpload.AllocateFromPage(NumBytes, 512);
	__m128 VectorValue = _mm_set1_ps(Value.Float);
	SIMDMemFill(TempSpace.DataPtr, VectorValue, RMath::DivideByMultiple(NumBytes, 16));
	CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void RCommandList::TransitionResource(RGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate /*= false*/)
{
	D3D12_RESOURCE_STATES OldState = Resource.UsageState;

	if (Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
	{
		assert((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
		assert((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
	}

	if (OldState != NewState)
	{
		assert(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& BarrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = Resource.GetResource();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = OldState;
		BarrierDesc.Transition.StateAfter = NewState;

		// Check to see if we already started the transition
		if (NewState == Resource.TransitioningState)
		{
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			Resource.TransitioningState = (D3D12_RESOURCE_STATES)-1;
		}
		else
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		Resource.UsageState = NewState;
	}
	else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		InsertUAVBarrier(Resource, FlushImmediate);

	if (FlushImmediate || NumBarriersToFlush == 16)
		FlushResourceBarriers();
}

void RCommandList::BeginResourceTransition(RGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate /*= false*/)
{
	// If it's already transitioning, finish that transition
	if (Resource.TransitioningState != (D3D12_RESOURCE_STATES)-1)
		TransitionResource(Resource, Resource.TransitioningState);

	D3D12_RESOURCE_STATES OldState = Resource.UsageState;

	if (OldState != NewState)
	{
		assert(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& BarrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = Resource.GetResource();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = OldState;
		BarrierDesc.Transition.StateAfter = NewState;

		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

		Resource.TransitioningState = NewState;
	}

	if (FlushImmediate || NumBarriersToFlush == 16)
		FlushResourceBarriers();
}

void RCommandList::InsertUAVBarrier(RGpuResource& Resource, bool FlushImmediate /*= false*/)
{
	assert(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER& BarrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.UAV.pResource = Resource.GetResource();

	if (FlushImmediate)
		FlushResourceBarriers();
}

void RCommandList::InsertAliasBarrier(RGpuResource& Before, RGpuResource& After, bool FlushImmediate /*= false*/)
{
	assert(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER& BarrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
	BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

	if (FlushImmediate)
		FlushResourceBarriers();
}

void RCommandList::SetPipelineState(const RPipelineState& PSO)
{
	ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
	if (PipelineState == CurPipelineState)
		return;

	CmdList->SetPipelineState(PipelineState);
	CurPipelineState = PipelineState;
}

void RCommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr)
{
	if (CurrentDescriptorHeaps[Type] != HeapPtr)
	{
		CurrentDescriptorHeaps[Type] = HeapPtr;
		BindDescriptorHeaps();
	}
}

void RCommandList::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[])
{
	bool AnyChanged = false;

	for (UINT i = 0; i < HeapCount; ++i)
	{
		if (CurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
		{
			CurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
			AnyChanged = true;
		}
	}

	if (AnyChanged)
		BindDescriptorHeaps();
}

void RCommandList::BindDescriptorHeaps()
{
	UINT NonNullHeaps = 0;
	ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* HeapIter = CurrentDescriptorHeaps[i];
		if (HeapIter != nullptr)
			HeapsToBind[NonNullHeaps++] = HeapIter;
	}

	if (NonNullHeaps > 0)
		CmdList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

void RCommandListGraphics::ClearUAV(RGpuBuffer& Target)
{
	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	const UINT ClearColor[4] = {};
	CmdList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void RCommandListGraphics::ClearUAV(RColorBuffer& Target)
{
	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

	//TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
	const float* ClearColor = Target.GetClearColor().GetPtr();
	CmdList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void RCommandListGraphics::SetRootSignature(const RRootSignature& RootSig)
{
	if (RootSig.GetSignature() == CurGraphicsRootSignature)
		return;

	CmdList->SetGraphicsRootSignature(CurGraphicsRootSignature = RootSig.GetSignature());

	DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
	DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
}

void RCommandListGraphics::SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth /*= 0.0f*/, FLOAT maxDepth /*= 1.0f*/)
{
	D3D12_VIEWPORT vp;
	vp.Width = w;
	vp.Height = h;
	vp.MinDepth = minDepth;
	vp.MaxDepth = maxDepth;
	vp.TopLeftX = x;
	vp.TopLeftY = y;
	CmdList->RSSetViewports(1, &vp);
}

void RCommandListGraphics::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
	assert(BufferData != nullptr && RMath::IsAligned(BufferData, 16));
	RLinearGpuMemory cb = MemoryAllocatorLinearUpload.AllocateFromPage(BufferSize);
	//SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	memcpy(cb.DataPtr, BufferData, BufferSize);
	CmdList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void RCommandListGraphics::SetBufferSRV(UINT RootIndex, const RGpuBuffer& SRV, UINT64 Offset /*= 0*/)
{
	assert((SRV.UsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
	CmdList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

void RCommandListGraphics::SetBufferUAV(UINT RootIndex, const RGpuBuffer& UAV, UINT64 Offset /*= 0*/)
{
	assert((UAV.UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
	CmdList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

void RCommandListGraphics::SetDynamicVertexBuffer(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData)
{
	assert(VBData != nullptr && RMath::IsAligned(VBData, 16));

	size_t BufferSize = RMath::AlignUp(NumVertices * VertexStride, 16);
	RLinearGpuMemory vb = MemoryAllocatorLinearUpload.AllocateFromPage(BufferSize);

	SIMDMemCopy(vb.DataPtr, VBData, BufferSize >> 4);

	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = vb.GpuAddress;
	VBView.SizeInBytes = (UINT)BufferSize;
	VBView.StrideInBytes = (UINT)VertexStride;

	CmdList->IASetVertexBuffers(Slot, 1, &VBView);
}

void RCommandListGraphics::SetDynamicIndexBuffer(size_t IndexCount, const uint16_t* IBData)
{
	assert(IBData != nullptr && RMath::IsAligned(IBData, 16));

	size_t BufferSize = RMath::AlignUp(IndexCount * sizeof(uint16_t), 16);
	RLinearGpuMemory ib = MemoryAllocatorLinearUpload.AllocateFromPage(BufferSize);

	SIMDMemCopy(ib.DataPtr, IBData, BufferSize >> 4);

	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.GpuAddress;
	IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
	IBView.Format = DXGI_FORMAT_R16_UINT;

	CmdList->IASetIndexBuffer(&IBView);
}

void RCommandListGraphics::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
	assert(BufferData != nullptr && RMath::IsAligned(BufferData, 16));
	RLinearGpuMemory cb = MemoryAllocatorLinearUpload.AllocateFromPage(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, RMath::AlignUp(BufferSize, 16) >> 4);
	CmdList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
}

void RCommandListGraphics::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation /*= 0*/, UINT StartInstanceLocation /*= 0*/)
{
	FlushResourceBarriers();
	DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(CmdList);
	DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(CmdList);
	CmdList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void RCommandListGraphics::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(CmdList);
	DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(CmdList);
	CmdList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}
