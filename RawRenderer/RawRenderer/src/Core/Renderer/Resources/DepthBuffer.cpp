#include "DepthBuffer.h"

#include "LLRenderer.h"

RDepthBuffer::RDepthBuffer(float DepthClearVal /*= 0.0f*/, uint8_t StencilClearVal /*= 0*/) : DepthClear(DepthClearVal), StencilClear(StencilClearVal)
{
	DSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	DSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	DSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	DSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	DepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	StencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

void RDepthBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, uint32_t NumSamples /*= 1*/, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr /*= D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN*/)
{
	D3D12_RESOURCE_DESC ResourceDesc = GetDesc(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ResourceDesc.SampleDesc.Count = NumSamples;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	for (int i = 0; i < 4; i++)
		ClearValue.Color[i] = DepthClear;
	CreateTextureResource(Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Format);
}

void RDepthBuffer::CreateDerivedViews(DXGI_FORMAT Format)
{
	ID3D12Resource* pResource = Resource.Get();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(Format);
	if (pResource->GetDesc().SampleDesc.Count == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	if (DSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		DSV[0] = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DSV[1] = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	SLLRenderer::GetDevice()->CreateDepthStencilView(pResource, &dsvDesc, DSV[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	SLLRenderer::GetDevice()->CreateDepthStencilView(pResource, &dsvDesc, DSV[1]);

	DXGI_FORMAT stencilReadFormat = GetStencilFormat(Format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (DSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			DSV[2] = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			DSV[3] = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		SLLRenderer::GetDevice()->CreateDepthStencilView(pResource, &dsvDesc, DSV[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		SLLRenderer::GetDevice()->CreateDepthStencilView(pResource, &dsvDesc, DSV[3]);
	}
	else
	{
		DSV[2] = DSV[0];
		DSV[3] = DSV[1];
	}

	if (DepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		DepthSRV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = GetDepthFormat(Format);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SLLRenderer::GetDevice()->CreateShaderResourceView(pResource, &SRVDesc, DepthSRV);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (StencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			StencilSRV = SLLRenderer::AllocateDescriptorCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SRVDesc.Format = stencilReadFormat;
		SLLRenderer::GetDevice()->CreateShaderResourceView(pResource, &SRVDesc, StencilSRV);
	}
}


DXGI_FORMAT RDepthBuffer::GetDSVFormat(DXGI_FORMAT Format)
{
	switch (Format)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_D16_UNORM;

	default:
		return Format;
	}
}

DXGI_FORMAT RDepthBuffer::GetDepthFormat(DXGI_FORMAT Format)
{
	switch (Format)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT RDepthBuffer::GetStencilFormat(DXGI_FORMAT Format)
{
	switch (Format)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

