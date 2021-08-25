#include "ColorBuffer.h"

#include "LLRenderer.h"

void RColorBuffer::CreateRTFromSwapChainBuffer(const std::wstring& Name, ID3D12Resource* BaseResource)
{
	AssociateWithResource(Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

	RTVHandle = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	SLLRenderer::GetDevice()->CreateRenderTargetView(Resource.Get(), nullptr, RTVHandle);
}

void RColorBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr /*= D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN*/)
{
	NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
	D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
	D3D12_RESOURCE_DESC ResourceDesc = GetDesc(Width, Height, 1, NumMips, Format, Flags);

	ResourceDesc.SampleDesc.Count = FragmentCount;
	ResourceDesc.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	ClearValue.Color[0] = ClearColor.R();
	ClearValue.Color[1] = ClearColor.G();
	ClearValue.Color[2] = ClearColor.B();
	ClearValue.Color[3] = ClearColor.A();

	CreateTextureResource(Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Format, 1, NumMips);
}

void RColorBuffer::CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr /*= D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN*/)
{
	D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
	D3D12_RESOURCE_DESC ResourceDesc = GetDesc(Width, Height, ArrayCount, 1, Format, Flags);

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	ClearValue.Color[0] = ClearColor.R();
	ClearValue.Color[1] = ClearColor.G();
	ClearValue.Color[2] = ClearColor.B();
	ClearValue.Color[3] = ClearColor.A();

	CreateTextureResource(Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Format, ArrayCount, 1);
}

void RColorBuffer::CreateDerivedViews(DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips /*= 1*/)
{
	assert(ArraySize == 1 || NumMips == 1, "We don't support auto-mips on texture arrays");

	NumMipMaps = NumMips - 1;

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

	RTVDesc.Format = Format;
	UAVDesc.Format = GetUAVFormat(Format);
	SRVDesc.Format = Format;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (ArraySize > 1)
	{
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.MipSlice = 0;
		RTVDesc.Texture2DArray.FirstArraySlice = 0;
		RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVDesc.Texture2DArray.MipSlice = 0;
		UAVDesc.Texture2DArray.FirstArraySlice = 0;
		UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.MipLevels = NumMips;
		SRVDesc.Texture2DArray.MostDetailedMip = 0;
		SRVDesc.Texture2DArray.FirstArraySlice = 0;
		SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
	}
	else if (FragmentCount > 1)
	{
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;

		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;

		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = NumMips;
		SRVDesc.Texture2D.MostDetailedMip = 0;
	}

	if (SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		RTVHandle = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		SRVHandle = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	ID3D12Resource* pResource = Resource.Get();

	// Create the render target view
	SLLRenderer::GetDevice()->CreateRenderTargetView(pResource, &RTVDesc, RTVHandle);

	// Create the shader resource view
	SLLRenderer::GetDevice()->CreateShaderResourceView(pResource, &SRVDesc, SRVHandle);

	if (FragmentCount > 1)
		return;

	// Create the UAVs for each mip level (RWTexture2D)
	for (uint32_t i = 0; i < NumMips; ++i)
	{
		if (UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			UAVHandle[i] = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SLLRenderer::GetDevice()->CreateUnorderedAccessView(pResource, nullptr, &UAVDesc, UAVHandle[i]);

		UAVDesc.Texture2D.MipSlice++;
	}
}
