#pragma once
#include <utility>
#include <DirectXMath.h>
#include "Core/Window.h"

#include <dinput.h>

class Mouse
{
	friend class SWindow;
	friend class SApplication;
public:

	static Mouse& Get()
	{
		static Mouse mouse;
		return mouse;
	}
	
	const DirectX::XMFLOAT2 GetRawOffset() const noexcept
	{
		return rawOffset;
	}
	const DirectX::XMFLOAT2 GetPositionDif() const noexcept
	{
		return positionDif;
	}

	const DirectX::XMFLOAT2 GetCursorPosition() const noexcept
	{
		return newPosition;
	}
	
	bool IsLButtonPressed() { return LButtonDown; }
	bool IsRButtonPressed() { return RButtonDown; }

	void CreateDIDevice(HWND hwnd);

	float sensitivity = .1f;

protected:

	void FlushPosOffset()
	{
		positionDif = { 0.f, 0.f };
		rawOffset = { 0.f, 0.f };
	}

	void OnUpdate(bool bCursorEnabled)
	{
		if (!bCursorEnabled)
		{
			MouseDevice->Acquire();
			bDeviceIsAcquired = true;
			MouseDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
			rawOffset = { MouseState.lX * -.0018f * sensitivity, MouseState.lY * -.0018f * sensitivity };
		}
		else
		{
			if (bDeviceIsAcquired)
				MouseDevice->Unacquire();
		}
	}

	void ZeroInput()
	{
		memset(&MouseState, 0, sizeof(DIMOUSESTATE2));

	}

	void MouseMoveEvent(WPARAM wParam, LPARAM lParam, SWindow& window);

	void RawMouseMoveEvent(LONG x, LONG y);

	void LMouseButtonDownEvent(WPARAM wParam, LPARAM lParam);

	void LMouseButtonUpEvent(WPARAM wParam, LPARAM lParam);

	void RMouseButtonDownEvent(WPARAM wParam, LPARAM lParam);

	void RMouseButtonUpEvent(WPARAM wParam, LPARAM lParam);

	void MouseWheelEvent(WPARAM keycode, LPARAM lParam);

	void MouseWheelPressedEvent(WPARAM wParam, LPARAM lParam);

private:

	Mouse() = default;

	~Mouse()
	{
		if (MouseDevice)
		{
			MouseDevice->Unacquire();
			MouseDevice->Release();
			MouseDevice = nullptr;
		}
		if (DirectInput)
		{
			DirectInput->Release();
			DirectInput = nullptr;
		}
	}

	bool LButtonDown = false;
	bool RButtonDown = false;
	bool MButtonDown = false;

	DirectX::XMFLOAT2 oldPosition;
	DirectX::XMFLOAT2 newPosition;
	DirectX::XMFLOAT2 positionDif;

	DirectX::XMFLOAT2 rawOffset;

	IDirectInput8A* DirectInput;

	IDirectInputDevice8A* MouseDevice;
	DIMOUSESTATE2 MouseState;

	bool bDeviceIsAcquired = false;

};