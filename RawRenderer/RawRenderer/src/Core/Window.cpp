#include <Windows.h>
#include "Window.h"

#include "Resource.h"

#include <sstream>
#include <optional>

#include "Input/Keyboard.h"
#include "Input/Mouse.h"

#include <windowsx.h>

#include "thirdParty/ImGUI/imgui_impl_win32.h"
#include "thirdParty/ImGUI/imgui.h"

#include "Utilities/Time.h"
#include "Events/EventHandler.h"

SWindow::WindowClass SWindow::WindowClass::winClassInstance;

LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void SWindow::OnKeyboardEvent(KeyboardEvent & event)
{
	if (event.IsPressed() && event.GetKey() == VK_SPACE)
	{
		EnableCursor();
		MouseCursorEvent cursorEnabled(MouseCursorEvent::CursorState::Shown);
		EventHandler::Dispatch(cursorEnabled);
		
	}
	if (event.IsPressed() && event.GetKey() == VK_CONTROL)
	{
		DisableCursor();
		MouseCursorEvent cursorDisabled(MouseCursorEvent::CursorState::Hidden);
		EventHandler::Dispatch(cursorDisabled);
	}
}

SWindow::SWindow(unsigned int width,unsigned int height, const char* name)
	: 
	width(width),
	height(height)
{
	assert(pSingleton == NULL);
	pSingleton = this;

	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;

	if (AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE) == 0)
	{
		__debugbreak();
	}

	hWnd = CreateWindow(
		L"DirectX Window",
		L"DirectX Window",
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		nullptr,
		nullptr,
		SWindow::WindowClass::GetInstance(),
		this
	);
	if (!hWnd)
	{
		__debugbreak;
	}

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01; //mouse
	rid.usUsage = 0x02; //mouse
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;
	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
	{
		__debugbreak();
	}


	/*  SW_HIDE             0
		SW_SHOWNORMAL       1
		SW_NORMAL           1
		SW_SHOWMINIMIZED    2
		SW_SHOWMAXIMIZED    3
		SW_MAXIMIZE         3
		SW_SHOWNOACTIVATE   4
		SW_SHOW             5
		SW_MINIMIZE         6
		SW_SHOWMINNOACTIVE  7
		SW_SHOWNA           8
		SW_RESTORE          9
		SW_SHOWDEFAULT      10
		SW_FORCEMINIMIZE    11
		SW_MAX              11*/

	ShowWindow(hWnd, SW_SHOWDEFAULT);


	EnableCursor();
	//HideCursor();


}

void SWindow::EnableCursor()
{
	bCursorEnabled = true;
	ShowCursor();
}

void SWindow::DisableCursor()
{
	bCursorEnabled = false;
	HideCursor();
}

void SWindow::ConfineCursor()
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	ClipCursor(&rect);
}

void SWindow::FreeCursor()
{
	ClipCursor(nullptr);
}

void SWindow::HideCursor()
{
	while (::ShowCursor(FALSE) >= 0);
	ConfineCursor();
}

void SWindow::ShowCursor()
{
	while (::ShowCursor(TRUE) < 0);
	FreeCursor();
}

SWindow::~SWindow()
{
	ImGui_ImplWin32_Shutdown();
	DestroyWindow(hWnd);
}


void SWindow::SetTitle(const std::wstring & title)
{
	if (SetWindowText(hWnd, title.c_str()) == 0)
	{
		assert(false);
	}
}



LRESULT WINAPI SWindow::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	//wm_nccreate has a pointer to a struct of info about created window
	if (msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		//extract ptr to window class
		SWindow* const pWnd = static_cast<SWindow*>(pCreate->lpCreateParams);
		//set WINAPI user data to store ptr to window
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd) );
		//set message proc to normal handler now that setup is finished
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&SWindow::GetPtrToMessageHandler));
		//forward message to window class handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);

}

LRESULT WINAPI SWindow::GetPtrToMessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//retrieve ptr to window class 
	SWindow* const pWnd = reinterpret_cast<SWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}


////////////////////////////MESSAGES///////////////////////////////////////////////////////////////////////////////////MESSAGES////////////////////////////////////////////////////////

LRESULT SWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		//return true;
	}
	
	switch (msg)
	{
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_KILLFOCUS:
		{
			Keyboard::Get().ResetKeyboardState();
			break;
		}

		case WM_ACTIVATE:
		{
			if (!bCursorEnabled)
			{
				if(wParam & WA_ACTIVE || wParam & WA_CLICKACTIVE)
				ConfineCursor();
			}
			else
			{
				FreeCursor();
			}
			break;
		}
		
		//RAW INPUT
		case WM_INPUT:
		{
			break;
		}

			//MOUSE
		case WM_MOUSEHWHEEL:
		{
			Mouse::Get().MouseWheelEvent(wParam, lParam);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			Mouse::Get().LMouseButtonDownEvent(wParam, lParam);
			break;
		}
		case WM_LBUTTONUP:
		{
			Mouse::Get().LMouseButtonUpEvent(wParam, lParam);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			Mouse::Get().RMouseButtonDownEvent(wParam, lParam);
			break;
		}
		case WM_RBUTTONUP:
		{
			Mouse::Get().RMouseButtonUpEvent(wParam, lParam);
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (ImGui::GetIO().WantCaptureMouse)
			{
				break;
			}

			/*Mouse::Get().MouseMoveEvent( wParam,  lParam, *this);
			break;*/
		}
		case WM_MBUTTONDOWN:
		{
			Mouse::Get().MouseWheelPressedEvent(wParam, lParam);
			break;
		}

			//KEYBOARD
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			if (ImGui::GetIO().WantCaptureKeyboard)
			{
				break;
			}
			if (static_cast<unsigned char>(wParam) == VK_ESCAPE)
			{
				PostQuitMessage(0);
				return 0;
			}
			Keyboard::Get().OnKeyPressed(static_cast<unsigned char>(wParam));
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			if (ImGui::GetIO().WantCaptureKeyboard)
			{
				break;
			}
			Keyboard::Get().OnKeyReleased((unsigned char)wParam);
			break;
		}
		case WM_CHAR:
		{
			Keyboard::Get().OnChar((unsigned char)wParam);
			break;
		}

	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

SWindow* SWindow::pSingleton = nullptr;

std::optional<int> SWindow::ProcessMessage()
{
	MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			return message.wParam;
		}

		TranslateMessage(&message);
		DispatchMessage(&message);

	}

	return std::nullopt;
}

///////////////////////////////////////////////////////////////////////////////////////WINCLASS/////////////////////////////////////////////////////


SWindow::WindowClass::WindowClass() noexcept
	: hInst(GetModuleHandle(nullptr))
{
	WNDCLASSEX winClass;
	winClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	winClass.lpfnWndProc = HandleMsgSetup;
	winClass.cbClsExtra = 0;
	winClass.cbWndExtra = 0;
	winClass.hInstance = SWindow::WindowClass::GetInstance();
	winClass.hIcon = LoadIcon(SWindow::WindowClass::GetInstance(), MAKEINTRESOURCE(IDI_RAWRENDERER));
	winClass.hCursor = LoadCursor(nullptr, IDC_ARROW);/*LoadCursor(hInst, MAKEINTRESOURCE(102));*/
	winClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);/*nullptr*/;
	winClass.lpszMenuName = nullptr;
	winClass.lpszClassName = SWindow::WindowClass::GetName(); 
	winClass.hIconSm = LoadIcon(SWindow::WindowClass::GetInstance(), MAKEINTRESOURCE(IDI_SMALL));/*nullptr*/;
	winClass.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&winClass);
}

SWindow::WindowClass::~WindowClass()
{
	UnregisterClass(SWindow::WindowClass::GetName(), SWindow::WindowClass::GetInstance());
}






