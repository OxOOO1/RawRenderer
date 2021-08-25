#include "Texture.h"

#include "LLRenderer.h"



//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
size_t BitsPerPixel(_In_ DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
		return 32;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		return 24;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 16;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
		return 12;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;

	default:
		return 0;
	}
}

static UINT BytesPerPixel(DXGI_FORMAT Format)
{
	return (UINT)BitsPerPixel(Format) / 8;
};


void RTexture::Create(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData)
{
	UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = Width;
	texDesc.Height = (UINT)Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	ASSERTHR(SLLRenderer::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		UsageState, nullptr, IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));

#ifdef _DEBUG
	Resource->SetName(L"Texture");
#endif

	D3D12_SUBRESOURCE_DATA texResource;
	texResource.pData = InitData;
	texResource.RowPitch = Pitch;// *BytesPerPixel(Format);
	texResource.SlicePitch = texResource.RowPitch * Height;

	RCommandList::UploadToTextureImmediate(*this, 1, &texResource);

	if (DescriptorHandleCPU.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		//DescriptorHandleCPU = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DescriptorHandleCPU = SLLRenderer::AllocateDescriptorTextureSRVCPU();
	}
		
	SLLRenderer::GetDevice()->CreateShaderResourceView(Resource.Get(), nullptr, DescriptorHandleCPU);
}

namespace RTextureManager
{
	std::string RootPath = "";
	std::map< std::string, std::unique_ptr<RManagedTexture> > TextureCache;
}

void RTextureManager::Initialize(const std::string& TextureLibRoot)
{
	RootPath = TextureLibRoot;
}

void RTextureManager::Shutdown(void)
{
	TextureCache.clear();
}


const RManagedTexture* RTextureManager::LoadFromFile(const std::string& filename, bool sRGB /*= false*/)
{
	/*std::string CatPath = fileName;

	const RManagedTexture* Tex = LoadDDSFromFile(CatPath + L".dds", sRGB);
	if (!Tex->IsValid())
		Tex = LoadTGAFromFile(CatPath + L".tga", sRGB);

	return Tex;*/

	//return LoadFromBitmap(filename);
	assert(false);
	return nullptr;
}

namespace RTextureManager
{
	std::pair<RManagedTexture*, bool> FindOrLoadTexture(const std::string& fileName)
	{
		static std::mutex s_Mutex;
		std::lock_guard<std::mutex> Guard(s_Mutex);

		auto iter = TextureCache.find(fileName);

		// If it's found, it has already been loaded or the load process has begun
		if (iter != TextureCache.end())
			return std::make_pair(iter->second.get(), false);

		RManagedTexture* NewTexture = new RManagedTexture(fileName);
		TextureCache[fileName].reset(NewTexture);

		// This was the first time it was requested, so indicate that the caller must read the file
		return std::make_pair(NewTexture, true);
	}
}

RManagedTexture* RTextureManager::LoadFromBitmap(const std::string& fileName, DXGI_FORMAT format)
{
	auto ManagedTex = FindOrLoadTexture(fileName);

	RManagedTexture* texture = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		texture->WaitForLoad();
		return texture;
	}

	texture->CreateFromBitmap(RBitmap::FromFile(fileName), format);
	return texture;
}

const RManagedTexture* RTextureManager::LoadDDSFromFile(const std::string& fileName, bool sRGB /*= false*/)
{
	assert(false);
	return nullptr;
}

const RManagedTexture* RTextureManager::LoadTGAFromFile(const std::string& fileName, bool sRGB /*= false*/)
{
	assert(false);
	return nullptr;
}

const RManagedTexture* RTextureManager::LoadPIXImageFromFile(const std::string& fileName)
{
	assert(false);
	return nullptr;
}

const RTexture& RTextureManager::GetBlackTex2D(void)
{
	auto ManagedTex = FindOrLoadTexture("DefaultBlackTexture");

	RManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return *ManTex;
	}

	uint32_t BlackPixel = 0;
	ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &BlackPixel);
	return *ManTex;
}

const RTexture& RTextureManager::GetWhiteTex2D(void)
{
	auto ManagedTex = FindOrLoadTexture("DefaultWhiteTexture");

	RManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return *ManTex;
	}

	uint32_t WhitePixel = 0xFFFFFFFFul;
	ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &WhitePixel);
	return *ManTex;
}

namespace RTextureManager
{
	const RTexture& GetMagentaTex2D(void)
	{
		auto ManagedTex = FindOrLoadTexture("DefaultMagentaTexture");

		RManagedTexture* ManTex = ManagedTex.first;
		const bool RequestsLoad = ManagedTex.second;

		if (!RequestsLoad)
		{
			ManTex->WaitForLoad();
			return *ManTex;
		}

		uint32_t MagentaPixel = 0x00FF00FF;
		ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &MagentaPixel);
		return *ManTex;
	}
}


RManagedTexture::RManagedTexture(const std::string& FileName) : MapKey(FileName)
{

}

void RManagedTexture::WaitForLoad(void) const
{
	volatile D3D12_CPU_DESCRIPTOR_HANDLE& VolHandle = (volatile D3D12_CPU_DESCRIPTOR_HANDLE&)DescriptorHandleCPU;
	volatile bool& VolValid = (volatile bool&)bIsValid;
	while (VolHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN && VolValid)
		std::this_thread::yield();
}

void RManagedTexture::SetToInvalidTexture(void)
{
	DescriptorHandleCPU = RTextureManager::GetMagentaTex2D().GetSRV();
	bIsValid = false;
}
