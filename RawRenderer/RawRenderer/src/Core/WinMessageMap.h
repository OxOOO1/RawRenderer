#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>

class WindowsMessageMap
{
public:
	WindowsMessageMap();
	std::string operator() (DWORD msg, LPARAM lParam, WPARAM wp) const;

private:

	std::unordered_map<DWORD, std::string> map;
};

