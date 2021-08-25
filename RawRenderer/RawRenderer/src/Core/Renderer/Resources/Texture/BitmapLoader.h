#pragma once

#include <cassert>
#include <memory>
#include <string>



class RBitmap
{
public:

	static std::shared_ptr<RBitmap> FromFile(const std::string& filename, int channels = 4);

	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }
	int GetNumChannels() const { return NumChannels; }
	int GetBytesPerPixel() const { return NumChannels * (bHDR ? sizeof(float) : sizeof(unsigned char)); }
	int GetPitch() const { return Width * GetBytesPerPixel(); }

	bool IsHDR() const { return bHDR; }

	template<typename T>
	const T* TGetData() const
	{
		return reinterpret_cast<const T*>(Data.get());
	}

	const unsigned char* GetData()
	{
		return Data.get();
	}

private:

	int Width = 0;
	int Height = 0;
	int NumChannels = 0;
	bool bHDR{ false };
	std::unique_ptr<unsigned char> Data;
};