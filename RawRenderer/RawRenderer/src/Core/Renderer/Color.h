#pragma once

#include <DirectXMath.h>
#include "Utilities/Math/Misc.h"

using namespace DirectX;

class RColor
{
public:
	RColor() : Value(g_XMOne) {}
	RColor(FXMVECTOR vec)
	{
		Value.v = vec;
	}
	RColor(const XMVECTORF32& vec)
	{
		Value = vec;
	}
	RColor(float r, float g, float b, float a = 1.0f)
	{
		Value.v = XMVectorSet(r, g, b, a);
	}
	RColor(uint16_t r, uint16_t g, uint16_t b, uint16_t a = 255, uint16_t bitDepth = 8)
	{
		Value.v = XMVectorScale(XMVectorSet(r, g, b, a), 1.0f / ((1 << bitDepth) - 1));
	}

	explicit RColor(uint32_t u32)
	{
		float r = (float)((u32 >> 0) & 0xFF);
		float g = (float)((u32 >> 8) & 0xFF);
		float b = (float)((u32 >> 16) & 0xFF);
		float a = (float)((u32 >> 24) & 0xFF);
		Value.v = XMVectorScale(XMVectorSet(r, g, b, a), 1.0f / 255.0f);
	}

	float R() const { return XMVectorGetX(Value); }
	float G() const { return XMVectorGetY(Value); }
	float B() const { return XMVectorGetZ(Value); }
	float A() const { return XMVectorGetW(Value); }

	bool operator==(const RColor& rhs) const { return XMVector4Equal(Value, rhs.Value); }
	bool operator!=(const RColor& rhs) const { return !XMVector4Equal(Value, rhs.Value); }

	void SetR(float r) { Value.f[0] = r; }
	void SetG(float g) { Value.f[1] = g; }
	void SetB(float b) { Value.f[2] = b; }
	void SetA(float a) { Value.f[3] = a; }

	float* GetPtr(void) { return reinterpret_cast<float*>(this); }
	float& operator[](int idx) { return GetPtr()[idx]; }

	void SetRGB(float r, float g, float b) { Value.v = XMVectorSelect(Value, XMVectorSet(r, g, b, b), g_XMMask3); }

	RColor ToSRGB() const;
	RColor FromSRGB() const;
	RColor ToREC709() const;
	RColor FromREC709() const;

	// Probably want to convert to sRGB or Rec709 first
	uint32_t R10G10B10A2() const;
	uint32_t R8G8B8A8() const;

	// Pack an HDR color into 32-bits
	uint32_t R11G11B10F(bool RoundToEven = false) const;

	uint32_t R9G9B9E5() const;

	operator XMVECTOR() const { return Value; }

private:
	XMVECTORF32 Value;
};