#pragma once
#include <Windows.h>
#include <bitset>
#include <unordered_map>
#include <functional>
#include "Events/KeyboardEvents.h"

class Keyboard
{
	friend class SWindow;
	friend class SApplication;
public:

	static Keyboard& Get()
	{
		static Keyboard keyboard;
		return keyboard;
	}

	static bool IsKeyPressed(unsigned char key)
	{
		return Keyboard::Get().IsKeyPressedImpl(key);
	}
	
public:


protected:
	void DispatchEventsFromQueue() noexcept;
	bool IsEventQueueEmpty() const noexcept;

	bool IsCharQueueEmpty() const noexcept;
	void FlushCharQueue() noexcept;
	void FlushAllEvents();

	void EnableAutorepeat() noexcept;
	void DisableAutorepeat() noexcept;
	bool IsAutorepeatEnabled() const noexcept;
	void FlushEventQueue() noexcept;
	void OnKeyPressed(unsigned char keycode) noexcept;
	void OnKeyReleased(unsigned char keycode) noexcept;
	void OnChar(char character) noexcept;

	void ResetKeyboardState() noexcept;

	template <typename T>
	static void TrimBuffer(std::queue<T>& buffer) noexcept;

private:
	Keyboard() = default;
	Keyboard(const Keyboard&) = delete;
	Keyboard& operator=(const Keyboard&) = delete;
	static constexpr unsigned int numOfKeys = 256u;
	std::bitset<numOfKeys> keystates;

	bool autorepeatEnabled = false;
	bool IsKeyPressedImpl(unsigned char keycode) const noexcept { return keystates[keycode]; };
	static constexpr unsigned int maxQueueSize = 16u;
	std::queue<KeyboardEvent> keyEventQueue;
	std::queue<KeyboardCharEvent> charEventQueue;

};


