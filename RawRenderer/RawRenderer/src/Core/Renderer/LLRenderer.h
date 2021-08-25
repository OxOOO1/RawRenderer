#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>
#include <string>
#include <DirectXMath.h>

#include "CmdListManager.h"
#include "CommandContext.h"
#include "Utilities/Utility.h"
#include "Utilities/Types.h"
#include "Resources/Texture/Texture.h"


#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

using namespace Microsoft::WRL;

class SLLRenderer
{
	friend class SApplication;
	friend class SRenderer;
public:

	static ComPtr<ID3D12Device>& GetDevice()
	{
		return pSingleton->Device;
	}

	static RColorBuffer& GetCurFramebuffer()
	{
		return pSingleton->SwapchainBuffers[pSingleton->CurSwapchainBufferIndex];
	}

	static D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1);
	static D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptorTextureSRVCPU(UINT Count = 1);

	static SCommandQueueManager& GetCmdQueueManager()
	{
		return pSingleton->CmdQueueManager;
	}

	static SCommandListManager& GetCmdListManager()
	{
		return pSingleton->CmdListManager;
	}

	static SLLRenderer& Get()
	{
		return *pSingleton;
	}

	static UINT2 GetOutputSize()
	{
		return pSingleton->OutputScreenSize;
	}

	static RDescriptorHeap& GetGPUViewsDescHeap()
	{
		return pSingleton->DescHeapCBVUAVSRVGpu;
	}

	static const D3D12_CPU_DESCRIPTOR_HANDLE& GetTexturesDescHandleCPU(uint Index = 0)
	{
		return pSingleton->DescHeapSRVTextures.GetHandle(Index).CPU;
	}

public:
	static constexpr UINT SWAPCHAIN_FRAME_COUNT = 3;
	static constexpr DXGI_FORMAT SWAPCHAIN_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	//static constexpr DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	//static constexpr DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	UINT2 OutputScreenSize;

private:
	RColorBuffer SwapchainBuffers[SWAPCHAIN_FRAME_COUNT];

	SCommandQueueManager CmdQueueManager;
	SCommandListManager CmdListManager;

	ComPtr<IDXGIFactory4> DxgiFactory;
	ComPtr<ID3D12Device> Device;
	//ComPtr<IDXGISwapChain3> SwapChain;
	IDXGISwapChain1* SwapChain;

	uint8_t CurSwapchainBufferIndex = 0;

	HWND HWnd;

	bool bEnableVSync = false;

private:
	D3D_ROOT_SIGNATURE_VERSION RootSignatureVersion;

	D3D_FEATURE_LEVEL D3DFeatureLevel;

	RDescriptorHeap DescHeapCBVUAVSRVGpu;

	SDescriptorAllocatorCPU DescHeapRTVCpu{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
	SDescriptorAllocatorCPU DescHeapDSVCpu{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
	SDescriptorAllocatorCPU DescHeapCBVUAVSRVCpu{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
	SDescriptorAllocatorCPU DescHeapSamplerCpu{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER };

	//All textures in one Heap for Bindless use
	RDescriptorHeap DescHeapSRVTextures;

	static SLLRenderer* pSingleton;

protected:

	SLLRenderer(HWND hWnd, UINT2 OutputScreenSize);

	void Terminate()
	{
		CmdQueueManager.CPUWaitForQueues();
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		SwapChain->SetFullscreenState(FALSE, nullptr);
#endif
	}

	void Shutdown();

	void BeginPresent();
	void Present();

	float GetFrameTime();
	float GetFrameRate();
	UINT64 GetFrameCount();

	void CreateSwapChain();

};

