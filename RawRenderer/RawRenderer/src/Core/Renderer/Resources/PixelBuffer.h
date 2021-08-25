#pragma once


#include "Core/Renderer/Resources/GpuResource.h"
#include "Core/Renderer/Resources/GpuBuffer.h"
#include <cstdint>
#include <string>

class RPixelBuffer : public RGpuResource
{
public:

	uint32_t GetWidth(void) const { return Width; }
	uint32_t GetHeight(void) const { return Height; }
	uint32_t GetDepth(void) const { return ArraySize; }
	const DXGI_FORMAT& GetFormat(void) const { return Format; }

	// Has no effect on Windows
	void SetBankRotation(uint32_t RotationAmount) { BankRotation = RotationAmount; }

	// Write the raw pixel buffer contents to a file
	// Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height }
	void ExportToFile(const std::wstring& FilePath);

protected:

	D3D12_RESOURCE_DESC GetDesc(uint32_t inWidth, uint32_t inHeight, uint32_t inDepthOrArraySize, uint32_t NumMips, DXGI_FORMAT inFormat, UINT Flags)
	{
		Width = inWidth;
		Height = inHeight;
		ArraySize = inDepthOrArraySize;
		Format = inFormat;

		D3D12_RESOURCE_DESC Desc = {};
		Desc.Alignment = 0;
		Desc.DepthOrArraySize = (UINT16)inDepthOrArraySize;
		Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		Desc.Flags = (D3D12_RESOURCE_FLAGS)Flags;
		Desc.Format = GetBaseFormat(Format);
		Desc.Height = (UINT)Height;
		Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		Desc.MipLevels = (UINT16)NumMips;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Width = (UINT64)Width;
		return Desc;
	}

	void AssociateWithResource(const std::wstring& Name, ID3D12Resource* inResource, D3D12_RESOURCE_STATES CurrentState);

	void CreateTextureResource(const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
		D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
	static size_t BytesPerPixel(DXGI_FORMAT Format);

	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t ArraySize = 0;
	DXGI_FORMAT Format{ DXGI_FORMAT_UNKNOWN };
	uint32_t BankRotation = 0;
};