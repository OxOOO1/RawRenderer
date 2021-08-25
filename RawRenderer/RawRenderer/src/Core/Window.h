#pragma once

#include <Windows.h>
#include <optional>
#include <memory>
#include <string>
#include "Events/Event.h"
#include "Events/KeyboardEvents.h"
#include "Events/WindowEvents.h"

constexpr const char* window_Name = "Direct Window\0";


class SWindow
{
public:
	
	void OnEvent(Event& event)
	{
		if (event.CheckTypeAndHandle(EventType::KeyboardEvent))
		{
			OnKeyboardEvent(reinterpret_cast<KeyboardEvent&>(event));
		}
	}

	void OnKeyboardEvent(KeyboardEvent& event);

private:

	class WindowClass
	{
	public:
		static const wchar_t* GetName() noexcept { return wndClassName; };
		static HINSTANCE GetInstance() noexcept { return Get().hInst; }
		static WindowClass& Get()
		{
			return winClassInstance;
		}
	private:
		WindowClass() noexcept;
		~WindowClass();
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator=(const WindowClass&) = delete;
		static constexpr const wchar_t* wndClassName = L"DirectX Window";
		static WindowClass winClassInstance;
		HINSTANCE hInst;
	};

public:
	SWindow(unsigned int width,unsigned int height, const char* name);
	~SWindow();
	SWindow(const SWindow&) = delete;
	SWindow& operator=(const SWindow&) = delete;
	void SetTitle(const std::wstring& title);
	HWND GetHandle() const noexcept { return hWnd; }
	std::pair<int, int> GetSize() const { return { width, height }; }
	inline void SetSize(int _width, int _height) { width = _width; height = _height; }
	static std::optional<int> ProcessMessage();
	void EnableCursor();
	void DisableCursor();
private:
	void ConfineCursor();
	void FreeCursor();
	void HideCursor();
	void ShowCursor();
	static LRESULT WINAPI HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT WINAPI GetPtrToMessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
public:
	bool bCursorEnabled = false;
	
private:
	unsigned int width, height;
	HWND hWnd;
	std::vector<char> rawBuffer;

private:
	static SWindow* pSingleton;

};