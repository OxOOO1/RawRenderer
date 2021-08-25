#pragma once

#include "PixelBuffer.h"
#include "Color.h"

class RColorBuffer : public RPixelBuffer
{

public:
	RColorBuffer(RColor ClearColor = RColor(0.0f, 0.0f, 0.0f, 0.0f))
		: ClearColor(ClearColor)
	{
		SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		std::memset(UAVHandle, 0xFF, sizeof(UAVHandle));
	}

	// Create a color buffer from a swap chain buffer.  Unordered access is restricted.
	void CreateRTFromSwapChainBuffer(const std::wstring& Name, ID3D12Resource* BaseResource);

	// Create a color buffer.  If an address is supplied, memory will not be allocated.
	// The vmem address allows you to alias buffers (which can be especially useful for
	// reusing ESRAM across a frame.)
	void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
		DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
		DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return SRVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const { return RTVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return UAVHandle[0]; }

	void SetClearColor(RColor ClearColor) { ClearColor = ClearColor; }

	void SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples)
	{
		assert(NumCoverageSamples >= NumColorSamples);
		FragmentCount = NumColorSamples;
		SampleCount = NumCoverageSamples;
	}

	RColor GetClearColor(void) const { return ClearColor; }

	//// This will work for all texture sizes, but it's recommended for speed and quality
	//// that you use dimensions with powers of two (but not necessarily square.)  Pass
	//// 0 for ArrayCount to reserve space for mips at creation time.
	//void GenerateMipMaps(RCommandContext& BaseContext)
	//{
	//	if (NumMipMaps == 0)
	//		return;

	//	RComputeContext& Context = BaseContext.GetComputeContext();

	//	Context.SetRootSignature(Graphics::g_GenerateMipsRS);

	//	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	Context.SetDynamicDescriptor(1, 0, SRVHandle);

	//	for (uint32_t TopMip = 0; TopMip < NumMipMaps; )
	//	{
	//		uint32_t SrcWidth = Width >> TopMip;
	//		uint32_t SrcHeight = Height >> TopMip;
	//		uint32_t DstWidth = SrcWidth >> 1;
	//		uint32_t DstHeight = SrcHeight >> 1;

	//		// Determine if the first downsample is more than 2:1.  This happens whenever
	//		// the source width or height is odd.
	//		uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;
	//		if (Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
	//			Context.SetPipelineState(Graphics::g_GenerateMipsGammaPSO[NonPowerOfTwo]);
	//		else
	//			Context.SetPipelineState(Graphics::g_GenerateMipsLinearPSO[NonPowerOfTwo]);

	//		// We can downsample up to four times, but if the ratio between levels is not
	//		// exactly 2:1, we have to shift our blend weights, which gets complicated or
	//		// expensive.  Maybe we can update the code later to compute sample weights for
	//		// each successive downsample.  We use _BitScanForward to count number of zeros
	//		// in the low bits.  Zeros indicate we can divide by two without truncating.
	//		uint32_t AdditionalMips;
	//		_BitScanForward((unsigned long*)&AdditionalMips,
	//			(DstWidth == 1 ? DstHeight : DstWidth) | (DstHeight == 1 ? DstWidth : DstHeight));

	//		uint32_t NumMips = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);

	//		if (TopMip + NumMips > NumMipMaps)
	//			NumMips = NumMipMaps - TopMip;

	//		// These are clamped to 1 after computing additional mips because clamped
	//		// dimensions should not limit us from downsampling multiple times.  (E.g.
	//		// 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
	//		if (DstWidth == 0)
	//			DstWidth = 1;
	//		if (DstHeight == 0)
	//			DstHeight = 1;

	//		Context.SetConstants(0, TopMip, NumMips, 1.0f / DstWidth, 1.0f / DstHeight);
	//		Context.SetDynamicDescriptors(2, 0, NumMips, UAVHandle + TopMip + 1);
	//		Context.Dispatch2D(DstWidth, DstHeight);

	//		Context.InsertUAVBarrier(*this);

	//		TopMip += NumMips;
	//	}

	//	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
	//		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//}


protected:

	D3D12_RESOURCE_FLAGS CombineResourceFlags(void) const
	{
		D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

		if (Flags == D3D12_RESOURCE_FLAG_NONE && FragmentCount == 1)
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
	}

	// Compute the number of texture levels needed to reduce to 1x1.  This uses
	// _BitScanReverse to find the highest set bit.  Each dimension reduces by
	// half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
	// as the dimension 511 (0x1FF).
	static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
	{
		uint32_t HighBit;
		_BitScanReverse((unsigned long*)&HighBit, Width | Height);
		return HighBit + 1;
	}

	void CreateDerivedViews(DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

	RColor ClearColor;
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle[12];
	uint32_t NumMipMaps = 0; 
	uint32_t FragmentCount = 1;
	uint32_t SampleCount = 1;
};
