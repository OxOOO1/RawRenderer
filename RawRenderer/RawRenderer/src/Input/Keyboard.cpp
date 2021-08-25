#include "Keyboard.h"

#include "Events/EventHandler.h"

void Keyboard::DispatchEventsFromQueue() noexcept
{
	if (!keyEventQueue.empty())
	{
		if (EventHandler::Dispatch(keyEventQueue.front()))
		{
			keyEventQueue.pop();
			DispatchEventsFromQueue();
		}
	}
	if (!charEventQueue.empty())
	{
		if (EventHandler::Dispatch(charEventQueue.front()))
		{
			charEventQueue.pop();
			DispatchEventsFromQueue();
		}
	}
}

bool Keyboard::IsEventQueueEmpty() const noexcept
{
	return keyEventQueue.empty();
}

void Keyboard::FlushEventQueue() noexcept
{
	keyEventQueue = std::queue<KeyboardEvent>();
}

bool Keyboard::IsCharQueueEmpty() const noexcept
{
	return charEventQueue.empty();
}

void Keyboard::FlushCharQueue() noexcept
{
	charEventQueue = std::queue<KeyboardCharEvent>();
}

void Keyboard::FlushAllEvents()
{
	FlushCharQueue();
	FlushEventQueue();
}

void Keyboard::EnableAutorepeat() noexcept
{
	autorepeatEnabled = true;
}

void Keyboard::DisableAutorepeat() noexcept
{
	autorepeatEnabled = false;
}

bool Keyboard::IsAutorepeatEnabled() const noexcept
{
	return autorepeatEnabled;
}

void Keyboard::OnKeyPressed(unsigned char keycode) noexcept
{
	keystates[keycode] = true;

	KeyboardEvent event(KeyboardEvent::Type::Pressed, keycode);
	if (!EventHandler::Dispatch(event))
	{
		keyEventQueue.push(event);
		TrimBuffer(keyEventQueue);
	}
	
}

void Keyboard::OnKeyReleased(unsigned char keycode) noexcept
{
	keystates[keycode] = false;

	KeyboardEvent event(KeyboardEvent::Type::Released, keycode);
	if (!EventHandler::Dispatch(event))
	{
		keyEventQueue.push(event);
		TrimBuffer(keyEventQueue);
	}
}

void Keyboard::OnChar(char character) noexcept
{
	KeyboardCharEvent event(character);
	if (!EventHandler::Dispatch(event))
	{
		charEventQueue.push(event);
		TrimBuffer(keyEventQueue);
	}
}

void Keyboard::ResetKeyboardState() noexcept
{
	keystates.reset();
}

template<typename T>
inline void Keyboard::TrimBuffer(std::queue<T>& queue) noexcept
{
	while (queue.size() > maxQueueSize)
	{
		queue.pop();
	}
}