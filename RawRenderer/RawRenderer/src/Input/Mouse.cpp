#include "Mouse.h"

#include <windowsx.h>

#include "Events/MouseEvents.h"
#include "Events/EventHandler.h"

#include "Utilities/Utility.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Mouse::MouseMoveEvent(WPARAM keycode, LPARAM lParam, SWindow& window)
{
	
	oldPosition = { newPosition.x, newPosition.y };

	newPosition = { static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(window.GetSize().second - GET_Y_LPARAM(lParam)) };
	
	auto difX = newPosition.x - oldPosition.x;
	auto difY = newPosition.y - oldPosition.y;

	DirectX::XMStoreFloat2(&positionDif,
		DirectX::XMVectorAdd(
			DirectX::XMLoadFloat2(&positionDif),
			DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&newPosition), DirectX::XMLoadFloat2(&oldPosition))));

}

void Mouse::RawMouseMoveEvent(LONG xOffset, LONG yOffset)
{
	rawOffset = { rawOffset.x - (xOffset / (sensitivity * 100.f)), rawOffset.y - (yOffset / (sensitivity * 100.f)) };
}

void Mouse::LMouseButtonDownEvent(WPARAM wParam, LPARAM lParam)
{

	MouseEvent::Type type;
	if (LButtonDown == true)
		type = MouseEvent::Type::Repeated;
	else type = MouseEvent::Type::Pressed;

	MouseEvent event(type, MouseEvent::Button::Left);
	EventHandler::Dispatch(event);
	LButtonDown = true;
}

void Mouse::LMouseButtonUpEvent(WPARAM wParam, LPARAM lParam)
{
	LButtonDown = false;
	MouseEvent event(MouseEvent::Type::Released, MouseEvent::Button::Left);
	EventHandler::Dispatch(event);
	
}

void Mouse::RMouseButtonDownEvent(WPARAM wParam, LPARAM lParam)
{
	MouseEvent::Type type;
	if (RButtonDown == true)
		type = MouseEvent::Type::Repeated;
	else type = MouseEvent::Type::Pressed;

	MouseEvent event(type, MouseEvent::Button::Right);
	EventHandler::Dispatch(event);
	RButtonDown = true;
}

void Mouse::RMouseButtonUpEvent(WPARAM wParam, LPARAM lParam)
{
	RButtonDown = false;
	MouseEvent event(MouseEvent::Type::Released, MouseEvent::Button::Right);
	EventHandler::Dispatch(event);
}

void Mouse::MouseWheelEvent(WPARAM keycode, LPARAM lParam)
{


}

void Mouse::MouseWheelPressedEvent(WPARAM wParam, LPARAM lParam)
{
	MouseEvent event(MouseEvent::Type::Pressed, MouseEvent::Button::Middle);
	EventHandler::Dispatch(event);

}

void Mouse::CreateDIDevice(HWND hWnd)
{
	ASSERTHR(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&DirectInput, nullptr));

	ASSERTHR(DirectInput->CreateDevice(GUID_SysMouse, &MouseDevice, nullptr));
	ASSERTHR(MouseDevice->SetDataFormat(&c_dfDIMouse2));
	ASSERTHR(MouseDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));

	ZeroInput();
}


