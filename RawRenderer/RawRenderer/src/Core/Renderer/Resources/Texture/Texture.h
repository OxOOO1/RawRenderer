#pragma once

#include "Resources/GpuResource.h"
#include <string>

#include "BitmapLoader.h"

//Texture to be loaded and used as SRV
class RTexture : public RGpuResource
{
	friend class RCommandList;

public:
	RTexture() { DescriptorHandleCPU.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
	RTexture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : DescriptorHandleCPU(Handle) {}

	// Create a 1-level 2D texture
	void Create(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData);
	void Create(size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData)
	{
		Create(Width, Width, Height, Format, InitData);
	}

	void CreateFromBitmap(std::shared_ptr<RBitmap> bitmap, DXGI_FORMAT format)
	{
		Create(bitmap->GetPitch(), bitmap->GetWidth(), bitmap->GetHeight(), format, bitmap->GetData());
	}
	void CreateTGAFromMemory(const void* memBuffer, size_t fileSize, bool sRGB)
	{

	}
	bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB)
	{

	}
	void CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize);

	//TODO: Descriptor Handle Leak fix
	virtual void Destroy() override
	{
		RGpuResource::Destroy();
		// This leaks descriptor handles.  We should really give it back to be reused.
		DescriptorHandleCPU.ptr = 0;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return DescriptorHandleCPU; }

	//bool operator!() { return DescriptorHandleCPU.ptr == 0; }

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandleCPU;

};

class RManagedTexture : public RTexture
{
public:
	RManagedTexture(const std::string& FileName);

	void WaitForLoad(void) const;
	void Unload(void);

	void SetToInvalidTexture(void);
	bool IsValid(void) const { return bIsValid; }

private:
	std::string MapKey;        // For deleting from the map later
	bool bIsValid{ true };
};

namespace RTextureManager
{
	void Initialize(const std::string& TextureLibRoot);
	void Shutdown(void);

	const RManagedTexture* LoadFromFile(const std::string& filename, bool sRGB = false);

	RManagedTexture* LoadFromBitmap(const std::string& fileName, DXGI_FORMAT format);
	const RManagedTexture* LoadDDSFromFile(const std::string& fileName, bool sRGB = false);
	const RManagedTexture* LoadTGAFromFile(const std::string& fileName, bool sRGB = false);
	const RManagedTexture* LoadPIXImageFromFile(const std::string& fileName);

	const RTexture& GetBlackTex2D(void);
	const RTexture& GetWhiteTex2D(void);
}

#pragma once




