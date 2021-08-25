#pragma once

#include "PixelBuffer.h"

class RDepthBuffer : public RPixelBuffer
{
public:
	RDepthBuffer(float DepthClearVal = 0.0f, uint8_t StencilClearVal = 0);

	void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		uint32_t NumSamples = 1, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
	
	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const { return DSV[0]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() const { return DSV[1]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() const { return DSV[2]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() const { return DSV[3]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() const { return DepthSRV; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() const { return StencilSRV; }

	float GetDepthClear() const { return DepthClear; }
	uint8_t GetStencilClear() const { return StencilClear; }

	static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);

private:

	void CreateDerivedViews(DXGI_FORMAT Format);

	float DepthClear;
	uint8_t StencilClear;
	D3D12_CPU_DESCRIPTOR_HANDLE DSV[4];
	D3D12_CPU_DESCRIPTOR_HANDLE DepthSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE StencilSRV;
};