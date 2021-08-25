#pragma once

#include "Event.h"
//#include "System/Input/Keyboard.h"


class KeyboardEvent : public Event
{
public:

	PasteTypeCheckMethods(KeyboardEvent)

		

	enum class Type
	{
		Pressed,
		Released,
		Repeated
	};

public:
	KeyboardEvent() = delete;

	KeyboardEvent(Type type, unsigned char charKey) noexcept
		: type(type), charKey(charKey) {}

	bool IsPressed() const noexcept { return type == Type::Pressed; }
	bool IsReleased() const noexcept { return type == Type::Released; }
	bool IsRepeated() const noexcept { return type == Type::Repeated; }

	unsigned char GetKey() const noexcept { return charKey; }

private:
	Type type;
	unsigned char charKey;
};

class KeyboardCharEvent : public Event
{
public:

	PasteTypeCheckMethods(KeyboardCharEvent)

public:
	KeyboardCharEvent() = delete;

	KeyboardCharEvent(unsigned char charKey) noexcept
		: charKey(charKey) {}

	unsigned char GetKey() const noexcept { return charKey; }

private:
	unsigned char charKey;
};