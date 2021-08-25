#include "LLRenderer.h"

#include "Utilities/Time.h"
#include "Resources/CommonResources.h"

#include <string>

SLLRenderer* SLLRenderer::pSingleton = nullptr;


D3D12_CPU_DESCRIPTOR_HANDLE SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
	switch (Type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return pSingleton->DescHeapCBVUAVSRVCpu.Allocate(Count);
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		pSingleton->DescHeapSamplerCpu.Allocate(Count);
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		return pSingleton->DescHeapRTVCpu.Allocate(Count);
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return pSingleton->DescHeapDSVCpu.Allocate(Count);
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES:
		break;
	default:
		break;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE SLLRenderer::AllocateDescriptorTextureSRVCPU(uint Count)
{
	return pSingleton->DescHeapSRVTextures.Allocate(Count).CPU;
}

void SLLRenderer::CreateSwapChain()
{
	//SwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = OutputScreenSize.x;
	swapChainDesc.Height = OutputScreenSize.y;
	swapChainDesc.Format = SWAPCHAIN_FORMAT;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = SWAPCHAIN_FRAME_COUNT;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;//

	//DxgiFactory->CreateSwapChain(CommandQueueGraphics.Get(), &sd, SwapChain.GetAddressOf());

	//ComPtr<IDXGISwapChain1> swapChain;
	ASSERTHR(SLLRenderer::DxgiFactory->CreateSwapChainForHwnd(CmdQueueManager.GetMainCommandQueue(), HWnd, &swapChainDesc, nullptr, nullptr, &SwapChain /*swapChain*/));

	//swapChain.As(&SwapChain);

}

SLLRenderer::SLLRenderer(HWND hWnd, UINT2 inOutputScreenSize)
	: OutputScreenSize(inOutputScreenSize), HWnd(hWnd)
{
	assert(pSingleton == nullptr);
	pSingleton = this;

	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	{
		ComPtr<ID3D12Debug> debugController;
		ASSERTHR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ASSERTHR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&DxgiFactory)));

	//Device
	ASSERTHR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

#if _DEBUG
	ID3D12InfoQueue* pInfoQueue = nullptr;
	if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
	{
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] =
		{
			// This occurs when there are uninitialized descriptors in a descriptor table, even when a
			// shader does not access the missing descriptors.  I find this is common when switching
			// shader permutations and not wanting to change much code to reorder resources.
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

			// Triggered when a shader does not export all color components of a render target, such as
			// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
			D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

			// This occurs when a descriptor table is unbound even when a shader does not access the missing
			// descriptors.  This is common with a root signature shared between disparate shaders that
			// don't all need the same types of resources.
			D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

			// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
			(D3D12_MESSAGE_ID)1008,
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		pInfoQueue->PushStorageFilter(&NewFilter);
		pInfoQueue->Release();
	}
#endif

	// Determine supported root signature version.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureFeature = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
		Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureFeature, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE));
		RootSignatureVersion = rootSignatureFeature.HighestVersion;
	}

	//Command Queue/Allocator/List
	CmdQueueManager.Create();

	CreateSwapChain();
	DxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

	//CurSwapchainBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	//for each swapchain buffer
	for (int i = 0; i < SWAPCHAIN_FRAME_COUNT; i++)
	{
		ComPtr<ID3D12Resource> Backbuffer;
		ASSERTHR(SwapChain->GetBuffer(i, IID_PPV_ARGS(&Backbuffer)));
		SwapchainBuffers[i].CreateRTFromSwapChainBuffer(L"SwapchainBuffer", Backbuffer.Detach());
	}

	DescHeapCBVUAVSRVGpu.Create( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, L"User Desc Heap", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE );

	DescHeapSRVTextures.Create(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, L"TexturesSRVHeap");
}


void SLLRenderer::Shutdown()
{
	RCommandList::DestroyAllContexts();
	CmdQueueManager.Shutdown();
	//GpuTimeManager::Shutdown();
	SwapChain->Release();

	RPipelineState::DestroyAll();
	RRootSignature::DestroyAll();
	SDescriptorAllocatorCPU::DestroyAll();

	//DestroyCommonState();
	//DestroyRenderingBuffers();

	for (UINT i = 0; i < SWAPCHAIN_FRAME_COUNT; ++i)
		SwapchainBuffers[i].Destroy();

	//g_PreDisplayBuffer.Destroy();

#if defined(_DEBUG)
	ID3D12DebugDevice* debugInterface;
	if (SUCCEEDED(Device->QueryInterface(&debugInterface)))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif
	
	if (Device != nullptr)
	{
		//Device->Release();
		Device = nullptr;
	}
		
}

namespace
{
	float FrameTime = 0.0f;
	int64_t FrameStartTick = 0;
	UINT64 FrameIndex = 0;

	bool bLimitTo30Hz = false;
	bool bDropRandomFrames = false;
}

#define CREATE_SIMD_FUNCTIONS( TYPE ) \
INLINE TYPE Round(TYPE s) { return TYPE(XMVectorRound(s)); }



void SLLRenderer::Present()
{
	/*if (g_bEnableHDROutput)
		PreparePresentHDR();
	else
		PreparePresentLDR();*/

	CurSwapchainBufferIndex = (CurSwapchainBufferIndex + 1) % SWAPCHAIN_FRAME_COUNT;

	UINT PresentInterval = bEnableVSync ? std::min(4, (int)roundf(FrameTime * 60.0f)) : 0;

	SwapChain->Present(PresentInterval, 0);

	int64_t CurrentTick = SystemTime::GetCurrentTick();

	if (bEnableVSync)
	{
		// With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
		// to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
		// long the previous frame should be displayed (i.e. the present interval.)
		FrameTime = (bLimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
	}
	else
	{
		// When running free, keep the most recent total frame time as the time step for
		// the next frame simulation.  This is not super-accurate, but assuming a frame
		// time varies smoothly, it should be close enough.
		FrameTime = (float)SystemTime::TimeBetweenTicks(FrameStartTick, CurrentTick);
	}

	FrameStartTick = CurrentTick;

	++FrameIndex;
	//TemporalEffects::Update((uint32_t)s_FrameIndex);

	CmdQueueManager.CPUWaitForQueues();

}

float SLLRenderer::GetFrameTime()
{
	return FrameTime;
}

float SLLRenderer::GetFrameRate()
{
	return FrameTime == 0.0f ? 0.0f : 1.0f / FrameTime;
}

UINT64 SLLRenderer::GetFrameCount()
{
	return FrameIndex;
}



