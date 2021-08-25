#pragma once

namespace RMath
{

	__forceinline float AngleFromXY(float x, float y)
	{
		const float Pi = 3.1415926535f;

		float theta = 0.0f;

		// Quadrant I or IV
		if (x >= 0.0f)
		{
			// If x = 0, then atanf(y/x) = +pi/2 if y > 0
			//                atanf(y/x) = -pi/2 if y < 0
			theta = atanf(y / x); // in [-pi/2, +pi/2]

			if (theta < 0.0f)
				theta += 2.0f * Pi; // in [0, 2*pi).
		}

		// Quadrant II or III
		else
			theta = atanf(y / x) + Pi; // in [0, 2*pi).

		return theta;
	}

	__forceinline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
	__forceinline float Max(float a, float b) { return a > b ? a : b; }
	__forceinline float Min(float a, float b) { return a < b ? a : b; }
	__forceinline float Clamp(float v, float a, float b) { return Min(Max(v, a), b); }

	template<typename T> static constexpr bool IsPowerOfTwo(T value)
	{
		return value != 0 && (value & (value - 1)) == 0;
	}
	template<typename T> static constexpr T RoundToPowerOfTwo(T value, int POT)
	{
		return (value + POT - 1) & -POT;
	}
	template<typename T> static constexpr T NumMipmapLevels(T width, T height)
	{
		T levels = 1;
		while ((width | height) >> levels) {
			++levels;
		}
		return levels;
	}

	template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask = 255)
	{
		return (T)(((size_t)value + mask) & ~mask);
	}

	template <typename T> __forceinline T AlignDownWithMask(T value, size_t mask)
	{
		return (T)((size_t)value & ~mask);
	}


	template <typename T> __forceinline T AlignUp(T value, size_t alignment)
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline T AlignDown(T value, size_t alignment)
	{
		return AlignDownWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline bool IsAligned(T value, size_t alignment)
	{
		return 0 == ((size_t)value & (alignment - 1));
	}

	//Divide and round up
	template <typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
	{
		return (T)((value + alignment - 1) / alignment);
	}

	template <typename T> __forceinline bool IsDivisible(T value, T divisor)
	{
		return (value / divisor) * divisor == value;
	}

	__forceinline uint8_t Log2(uint64_t value)
	{
		unsigned long mssb; // most significant set bit
		unsigned long lssb; // least significant set bit

		// If perfect power of two (only one set bit), return index of bit.  Otherwise round up
		// fractional log by adding 1 to most signicant set bit's index.
		if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
			return uint8_t(mssb + (mssb == lssb ? 0 : 1));
		else
			return 0;
	}

	template <typename T> __forceinline T AlignPowerOfTwo(T value)
	{
		return value == 0 ? 0 : 1 << Log2(value);
	}

}