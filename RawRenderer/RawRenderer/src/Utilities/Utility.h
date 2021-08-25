#pragma once

#include <xmmintrin.h>
#include <Windows.h>
#include <string>

#define ASSERTHR( hr, ... ) \
        if (FAILED(hr)) \
            __debugbreak()

inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

void SIMDMemCopy(void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords);
void SIMDMemFill(void* __restrict Dest, __m128 FillVector, size_t NumQuadwords);

std::wstring MakeWStr(const std::string & str);