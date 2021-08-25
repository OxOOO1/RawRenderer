
#pragma once

#include <DirectXMath.h>

#define INLINE __forceinline


namespace RMath
{
	INLINE float Lerp(float a, float b, float t) { return a + (b - a) * t; }
	INLINE float Max(float a, float b) { return a > b ? a : b; }
	INLINE float Min(float a, float b) { return a < b ? a : b; }
	INLINE float Clamp(float v, float a, float b) { return Min(Max(v, a), b); }
}