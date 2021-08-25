#include "Random.h"

#include <DirectXPackedVector.h>

float RandomGenerator::RandomFrom0To1()
{
	return (float)(rand()) / (float)RAND_MAX;
}

float RandomGenerator::Random(float from, float to)
{
	return from + RandomFrom0To1()*(to - from);
}

DirectX::XMFLOAT3 RandomGenerator::Random(DirectX::XMFLOAT3 from, DirectX::XMFLOAT3 to)
{

	return { Random(from.x, to.x), Random(from.y, to.y), Random(from.z, to.z) };

}

DirectX::XMVECTOR RandomGenerator::RandomUnitVector()
{
	DirectX::XMVECTOR One = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.f);
	DirectX::XMVECTOR Zero = DirectX::XMVectorZero();

	while (1)
	{
		//Get random point in the cube
		DirectX::XMVECTOR v = DirectX::XMVectorSet(
			Random(-1.0f, 1.0f),
			Random(-1.f, 1.f),
			Random(-1.f, 1.f),
			0.f
		);

		//Ignore points outside the unit sphere in order to get an even distribution over the unit sphere.
		if (DirectX::XMVector3Greater(DirectX::XMVector3LengthSq(v), One))
			continue;

		return DirectX::XMVector3Normalize(v);
	}
}

void RandomGenerator::CreateRandomNumbersTexture1D(Rendering & renderer, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& dest)
{
	DirectX::XMFLOAT4 randomValues[1024];

	for (int i = 0; i < 1024; i++)
	{
		randomValues[i].x = Random(-1.f, 1.f);
		randomValues[i].y = Random(-1.f, 1.f);
		randomValues[i].z = Random(-1.f, 1.f);
		randomValues[i].w = Random(-1.f, 1.f);
		DirectX::XMStoreFloat4(&randomValues[i], DirectX::XMVector4Normalize(DirectX::XMLoadFloat4(&randomValues[i])));
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(DirectX::XMFLOAT4);
	initData.SysMemSlicePitch = 0u;

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0u;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	Microsoft::WRL::ComPtr<ID3D11Texture1D> randomTex;

	renderer.ThrowIfFailed(renderer.GetDevice()->CreateTexture1D(&texDesc, &initData, &randomTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = 1;
	viewDesc.Texture1D.MostDetailedMip = 0;


	renderer.ThrowIfFailed(renderer.GetDevice()->CreateShaderResourceView(randomTex.Get(), &viewDesc, &dest));


}

void RandomGenerator::CreateRandomNumbersTexture2D(Rendering & renderer, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& dest)
{
	
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0u;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.SysMemPitch = 256 * sizeof(DirectX::PackedVector::XMCOLOR);

	DirectX::PackedVector::XMCOLOR color[256*256];

	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			DirectX::XMFLOAT3 t{ RandomFrom0To1() ,RandomFrom0To1() ,RandomFrom0To1() };

			color[i * 256 + j] = DirectX::PackedVector::XMCOLOR(t.x,t.y,t.z,0.f);

		}
		
	}

	initData.pSysMem = color;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> randomTex;

	renderer.ThrowIfFailed(renderer.GetDevice()->CreateTexture2D(&texDesc, &initData, &randomTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture1D.MipLevels = 1;
	viewDesc.Texture1D.MostDetailedMip = 0;


	renderer.ThrowIfFailed(renderer.GetDevice()->CreateShaderResourceView(randomTex.Get(), &viewDesc, &dest));


}

void RandomGenerator::CreateNoiseTexture(Rendering& renderer, UINT width, UINT height, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& dest)
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0u;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.SysMemPitch = width;

	std::vector<char> color;
	color.resize(width * height);

	for (int i = 0; i < width * height; i++)
	{
		color[i] = RandomGenerator::Random(0, 255);
	}

	initData.pSysMem = color.data();

	Microsoft::WRL::ComPtr<ID3D11Texture2D> randomTex;

	renderer.ThrowIfFailed(renderer.GetDevice()->CreateTexture2D(&texDesc, &initData, &randomTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture1D.MipLevels = 1;
	viewDesc.Texture1D.MostDetailedMip = 0;


	renderer.ThrowIfFailed(renderer.GetDevice()->CreateShaderResourceView(randomTex.Get(), &viewDesc, &dest));
}

float GaussianDistribution(float x, float y, float rho)
{
	float g = 1.0f / sqrtf(2.0f * DirectX::XM_PI * rho * rho);
	g *= expf(-(x*x + y * y) / (2 * rho*rho));

	return g;
}

void RandomGenerator::GaussGenerator(uint32_t arraySize, float* array, float fDeviation, float fMultiplier)//3.f, 1.25f;
{
	int i = 0;
	uint8_t center = (arraySize - 1) / 2;

	// Fill the center texel
	float weight = 1.0f * GaussianDistribution(0, 0, fDeviation);
	array[center] = weight;

	// Fill the right side
	for (i = 1; i < center + 1; i++)
	{
		weight = fMultiplier * GaussianDistribution((float)i, 0, fDeviation);
		array[center - i] = weight;
	}

	// Copy to the left side
	for (i = center + 1; i < arraySize; i++)
	{
		array[i] = array[arraySize - 1 - i];
	}

}