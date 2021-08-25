#include "Time.h"
#include <sstream>
#include <Windows.h>
#include "src/thirdParty/ImGUI/imgui.h"
#include "OutputPrint.h"
#include <profileapi.h>

#include "LLRenderer.h"

Time::Time()
{
	last = std::chrono::steady_clock::now();
	start = std::chrono::high_resolution_clock::now();
}

void Time::OnUpdate()
{
	const auto old = last;
	last = std::chrono::high_resolution_clock::now();
	dTime = (std::chrono::duration<float>(last - old)).count();
}

double SystemTime::sm_CpuTickDelta = 0.0;

void SystemTime::Initialize(void)
{
	LARGE_INTEGER frequency;
	assert(TRUE == QueryPerformanceFrequency(&frequency), "Unable to query performance counter frequency");
	sm_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
}

int64_t SystemTime::GetCurrentTick(void)
{
	LARGE_INTEGER currentTick;
	assert(TRUE == QueryPerformanceCounter(&currentTick), "Unable to query performance counter value");
	return static_cast<int64_t>(currentTick.QuadPart);
}

void SystemTime::BusyLoopSleep(float SleepTime)
{
	int64_t finalTick = (int64_t)((double)SleepTime / sm_CpuTickDelta) + GetCurrentTick();
	while (GetCurrentTick() < finalTick);
}

void RCpuTimer::PrintTime(const std::string& ProcessName)
{
	PRINTOUT(ProcessName.c_str() << "took: " << GetTime() * 1000.0 << "ms")
}

double RCpuTimer::GetDeltaTimeMs()
{
	return DeltaTime;
}

double RCpuTimer::DeltaTime = 0.;





namespace
{
	ID3D12QueryHeap* sm_QueryHeap = nullptr;
	ID3D12Resource* sm_ReadBackBuffer = nullptr;
	uint64_t* sm_TimeStampBuffer = nullptr;
	uint64_t sm_Fence = 0;
	uint32_t sm_MaxNumTimers = 0;
	uint32_t sm_NumTimers = 1;
	uint64_t sm_ValidTimeStart = 0;
	uint64_t sm_ValidTimeEnd = 0;
	double sm_GpuTickDelta = 0.0;
}

void RGpuTimeManager::Initialize(uint32_t MaxNumTimers /*= 4096*/)
{
	uint64_t GpuFrequency;
	SLLRenderer::GetCmdQueueManager().GetMainCommandQueue()->GetTimestampFrequency(&GpuFrequency);
	sm_GpuTickDelta = 1.0 / static_cast<double>(GpuFrequency);

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc;
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = sizeof(uint64_t) * MaxNumTimers * 2;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERTHR(SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sm_ReadBackBuffer)));
	sm_ReadBackBuffer->SetName(L"GpuTimeStamp Buffer");

	D3D12_QUERY_HEAP_DESC QueryHeapDesc;
	QueryHeapDesc.Count = MaxNumTimers * 2;
	QueryHeapDesc.NodeMask = 1;
	QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	ASSERTHR(SLLRenderer::GetDevice()->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&sm_QueryHeap)));
	sm_QueryHeap->SetName(L"GpuTimeStamp QueryHeap");

	sm_MaxNumTimers = (uint32_t)MaxNumTimers;
}

void RGpuTimeManager::Shutdown()
{
	if (sm_ReadBackBuffer != nullptr)
		sm_ReadBackBuffer->Release();

	if (sm_QueryHeap != nullptr)
		sm_QueryHeap->Release();
}

uint32_t RGpuTimeManager::NewTimer(void)
{
	return sm_NumTimers++;
}

void RGpuTimeManager::StartTimer(RCommandList& Context, UINT TimerIdx)
{
	Context.InsertTimeStamp(sm_QueryHeap, TimerIdx * 2);
}

void RGpuTimeManager::StopTimer(RCommandList& Context, UINT TimerIdx)
{
	Context.InsertTimeStamp(sm_QueryHeap, TimerIdx * 2 + 1);
}

void RGpuTimeManager::BeginReadBack(void)
{
	SCommandQueueManager::CPUWaitForFence(sm_Fence);
	D3D12_RANGE Range;
	Range.Begin = 0;
	Range.End = (sm_NumTimers * 2) * sizeof(uint64_t);
	ASSERTHR(sm_ReadBackBuffer->Map(0, &Range, reinterpret_cast<void**>(&sm_TimeStampBuffer)));

	sm_ValidTimeStart = sm_TimeStampBuffer[0];
	sm_ValidTimeEnd = sm_TimeStampBuffer[1];

	// On the first frame, with random values in the timestamp query heap, we can avoid a misstart.
	if (sm_ValidTimeEnd < sm_ValidTimeStart)
	{
		sm_ValidTimeStart = 0ull;
		sm_ValidTimeEnd = 0ull;
	}
}

void RGpuTimeManager::EndReadBack(void)
{
	// Unmap with an empty range to indicate nothing was written by the CPU
	D3D12_RANGE EmptyRange = {};
	sm_ReadBackBuffer->Unmap(0, &EmptyRange);
	sm_TimeStampBuffer = nullptr;

	RCommandList& Context = RCommandList::BeginNew();
	Context.InsertTimeStamp(sm_QueryHeap, 1);
	Context.ResolveTimeStamps(sm_ReadBackBuffer, sm_QueryHeap, sm_NumTimers * 2);
	Context.InsertTimeStamp(sm_QueryHeap, 0);
	sm_Fence = Context.ExecuteCmdListAndReleaseContext();
}

float RGpuTimeManager::GetTime(uint32_t TimerIdx)
{
	assert(sm_TimeStampBuffer != nullptr, "Time stamp readback buffer is not mapped");
	assert(TimerIdx < sm_NumTimers, "Invalid GPU timer index");

	uint64_t TimeStamp1 = sm_TimeStampBuffer[TimerIdx * 2];
	uint64_t TimeStamp2 = sm_TimeStampBuffer[TimerIdx * 2 + 1];

	if (TimeStamp1 < sm_ValidTimeStart || TimeStamp2 > sm_ValidTimeEnd || TimeStamp2 <= TimeStamp1)
		return 0.0f;

	return static_cast<float>(sm_GpuTickDelta * (TimeStamp2 - TimeStamp1));
}
