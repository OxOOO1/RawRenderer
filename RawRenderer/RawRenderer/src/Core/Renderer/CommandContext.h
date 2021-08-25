#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

#include "Resources/GpuResource.h"
#include "Resources/GpuBuffer.h"
#include "Resources/LinearAllocator.h"
#include "Core/Renderer/Descriptor.h"
#include "PipelineState.h"

#include "Resources/ColorBuffer.h"
#include "Resources/DepthBuffer.h"

#include <pix3.h>

struct RTypesUnion
{
	RTypesUnion(FLOAT f) : Float(f) {}
	RTypesUnion(UINT u) : Uint(u) {}
	RTypesUnion(INT i) : Int(i) {}

	void operator= (FLOAT f) { Float = f; }
	void operator= (UINT u) { Uint = u; }
	void operator= (INT i) { Int = i; }

	union
	{
		FLOAT Float;
		UINT Uint;
		INT Int;
	};
};

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )


class RCommandList;

class SCommandListManager
{
public:
	SCommandListManager(void) {}

	RCommandList* AllocateCmdList(D3D12_COMMAND_LIST_TYPE Type);

	void FreeContext(RCommandList* UsedContext);

	void DestroyAllContexts()
	{
		for (auto& ContextVec : ContextPoolArr)
		{
			ContextVec.clear();
		}
	}

private:
	std::vector<std::unique_ptr<RCommandList>> ContextPoolArr[4];
	std::queue<RCommandList*> AvailableContextsArr[4];
	std::mutex ContextAllocationMutex;
};

struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

class RCommandListGraphics;

class RCommandList : NonCopyable
{
	friend SCommandListManager;

private:
	RCommandList(D3D12_COMMAND_LIST_TYPE Type);

	void Reset();

public:

	~RCommandList()
	{
		if (CmdList != nullptr)
			CmdList->Release();
	}

	static void DestroyAllContexts(void);

	static RCommandList& BeginNew(const std::wstring ID = L"");

	// Flush existing commands to the GPU but keep the context alive
	uint64_t ExecuteCmdList(bool WaitForCompletion = false);

	// Flush existing commands and release the current context
	uint64_t ExecuteCmdListAndReleaseContext(bool WaitForCompletion = false);

	// Prepare to render by reserving a command list and command allocator
	void Initialize(void);

	RCommandListGraphics& GetGraphicsContext() {
		assert(Type != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
		return reinterpret_cast<RCommandListGraphics&>(*this);
	}

	/*RComputeContext& GetComputeContext() {
		return reinterpret_cast<ComputeContext&>(*this);
	}*/

	ID3D12GraphicsCommandList* GetCommandList() 
	{
		return CmdList;
	}

	void CopyBuffer(RGpuResource& Dest, RGpuResource& Src);
	void CopyBufferRegion(RGpuResource& Dest, size_t DestOffset, RGpuResource& Src, size_t SrcOffset, size_t NumBytes);
	void CopySubresource(RGpuResource& Dest, UINT DestSubIndex, RGpuResource& Src, UINT SrcSubIndex);
	void CopyCounter(RGpuResource& Dest, size_t DestOffset, RStructuredBuffer& Src);
	void ResetCounter(RStructuredBuffer& Buf, uint32_t Value = 0);

	RLinearGpuMemory ReserveUploadMemory(size_t SizeInBytes)
	{
		return MemoryAllocatorLinearUpload.AllocateFromPage(SizeInBytes);
	}

	static void UploadToTextureImmediate(RGpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	static void UploadToBufferImmediate(RGpuResource& Dest, const void* Data, size_t NumBytes, size_t Offset = 0);
	static void InitializeTextureArraySliceImmediate(RGpuResource& Dest, UINT SliceIndex, RGpuResource& Src);
	static void ReadbackTexture2DImmediate(RGpuResource& ReadbackBuffer, RPixelBuffer& SrcBuffer);

	void WriteBuffer(RGpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
	void FillBuffer(RGpuResource& Dest, size_t DestOffset, RTypesUnion Value, size_t NumBytes);

	void TransitionResource(RGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
	void BeginResourceTransition(RGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
	void InsertUAVBarrier(RGpuResource& Resource, bool FlushImmediate = false);
	void InsertAliasBarrier(RGpuResource& Before, RGpuResource& After, bool FlushImmediate = false);

	inline void FlushResourceBarriers(void)
	{
		if (NumBarriersToFlush > 0)
		{
			CmdList->ResourceBarrier(NumBarriersToFlush, ResourceBarrierBuffer);
			NumBarriersToFlush = 0;
		}
	}

	void InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx)
	{
		CmdList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
	}
	void ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries)
	{
		CmdList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
	}
	void PIXBeginEvent(const wchar_t* label)
	{
		::PIXBeginEvent(CmdList, 0, label);
	}
	void PIXEndEvent(void)
	{
		::PIXEndEvent(CmdList);
	}
	void PIXSetMarker(const wchar_t* label)
	{
		::PIXSetMarker(CmdList, 0, label);
	}

	void SetPipelineState(const RPipelineState& PSO);
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
	void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);

	void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
	{
		CmdList->SetPredication(Buffer, BufferOffset, Op);
	}

protected:

	void BindDescriptorHeaps();

	ID3D12GraphicsCommandList* CmdList = nullptr;
	ID3D12CommandAllocator* CurCmdAllocator = nullptr;

	ID3D12RootSignature* CurGraphicsRootSignature = nullptr;
	ID3D12PipelineState* CurPipelineState = nullptr;
	ID3D12RootSignature* CurComputeRootSignature = nullptr;

	RDynamicDescriptorHeap DynamicViewDescriptorHeap;        // HEAP_TYPE_CBV_SRV_UAV
	RDynamicDescriptorHeap DynamicSamplerDescriptorHeap;    // HEAP_TYPE_SAMPLER
	ID3D12DescriptorHeap* CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES /*4*/];

	D3D12_RESOURCE_BARRIER ResourceBarrierBuffer[16];
	UINT NumBarriersToFlush = 0;

	RLinearGpuMemoryAllocator MemoryAllocatorLinearUpload;
	RLinearGpuMemoryAllocator MemoryAllocatorLinearDefault;

	std::wstring IDString;
	void SetID(const std::wstring& ID) { IDString = ID; }

	D3D12_COMMAND_LIST_TYPE Type;
};

class RCommandListGraphics : public RCommandList
{
public:

	static RCommandListGraphics& BeginNew(const std::wstring& ID = L"")
	{
		return RCommandList::BeginNew(ID).GetGraphicsContext();
	}

	void ClearUAV(RGpuBuffer& Target);
	void ClearUAV(RColorBuffer& Target);
	void ClearRenderTarget(RColorBuffer& Target)
	{
		CmdList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), 0, nullptr);
	}
	void ClearDepth(RDepthBuffer& Target)
	{
		CmdList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetDepthClear(), Target.GetStencilClear(), 0, nullptr);
	}
	void ClearStencil(RDepthBuffer& Target)
	{
		CmdList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetDepthClear(), Target.GetStencilClear(), 0, nullptr);
	}
	void ClearDepthAndStencil(RDepthBuffer& Target)
	{
		CmdList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetDepthClear(), Target.GetStencilClear(), 0, nullptr);
	}

	void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
	{
		CmdList->BeginQuery(QueryHeap, Type, HeapIndex);
	}
	void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
	{
		CmdList->EndQuery(QueryHeap, Type, HeapIndex);
	}
	void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
	{
		CmdList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);

	}

	void SetRootSignature(const RRootSignature& RootSig);

	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
	{
		CmdList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
	}
	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV)
	{
		CmdList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
	}
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
	void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

	void SetViewport(const D3D12_VIEWPORT& vp)
	{
		CmdList->RSSetViewports(1, &vp);
	}
	void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
	void SetScissor(const D3D12_RECT& rect)
	{
		assert(rect.left < rect.right&& rect.top < rect.bottom);
		CmdList->RSSetScissorRects(1, &rect);
	}
	void SetScissor(UINT left, UINT top, UINT right, UINT bottom)
	{
		SetScissor(CD3DX12_RECT(left, top, right, bottom));
	}
	void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
	{
		assert(rect.left < rect.right&& rect.top < rect.bottom);
		CmdList->RSSetViewports(1, &vp);
		CmdList->RSSetScissorRects(1, &rect);
	}
	void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h)
	{
		SetViewport((float)x, (float)y, (float)w, (float)h);
		SetScissor(x, y, x + w, y + h);
	}
	void SetStencilRef(UINT StencilRef)
	{
		CmdList->OMSetStencilRef(StencilRef);
	}
	void SetBlendFactor(RColor BlendFactor)
	{
		CmdList->OMSetBlendFactor(BlendFactor.GetPtr());
	}
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
	{
		CmdList->IASetPrimitiveTopology(Topology);
	}

	void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
	{
		CmdList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
	}
	void SetConstant(UINT RootIndex, RTypesUnion Val, UINT Offset = 0)
	{
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, Val.Uint, Offset);
	}
	void SetConstants(UINT RootIndex, RTypesUnion X)
	{
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	}
	void SetConstants(UINT RootIndex, RTypesUnion X, RTypesUnion Y)
	{
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
	}
	void SetConstants(UINT RootIndex, RTypesUnion X, RTypesUnion Y, RTypesUnion Z)
	{
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
	}
	void SetConstants(UINT RootIndex, RTypesUnion X, RTypesUnion Y, RTypesUnion Z, RTypesUnion W)
	{
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
		CmdList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
	}
	void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
	{
		CmdList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
	}
	void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
	void SetBufferSRV(UINT RootIndex, const RGpuBuffer& SRV, UINT64 Offset = 0);
	void SetBufferUAV(UINT RootIndex, const RGpuBuffer& UAV, UINT64 Offset = 0);
	void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
	{
		CmdList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
	}

	void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
	}
	void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}
	void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
	}
	void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}

	void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
	{
		CmdList->IASetIndexBuffer(&IBView);
	}
	void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
	{
		SetVertexBuffers(Slot, 1, &VBView);
	}
	void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
	{
		CmdList->IASetVertexBuffers(StartSlot, Count, VBViews);
	}
	void SetDynamicVertexBuffer(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData);
	void SetDynamicIndexBuffer(size_t IndexCount, const uint16_t* IBData);
	void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

	void Draw(UINT VertexCount, UINT VertexStartOffset = 0)
	{
		DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
	}
	void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0)
	{
		DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
	}

	void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
		UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);

	void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
		INT BaseVertexLocation, UINT StartInstanceLocation);

	/*void DrawIndirect(RGpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0)
	{
		ExecuteIndirect(Graphics::DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
	}
	void ExecuteIndirect(RCommandSignature& CommandSig, RGpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
		uint32_t MaxCommands = 1, RGpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0)
	{

	}*/
};