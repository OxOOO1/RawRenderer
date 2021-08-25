#pragma once
#include "Event.h"


class MouseEvent : public Event
{
public:

	PasteTypeCheckMethods(MouseEvent)


	enum class Type
	{
		Pressed,
		Released,
		Repeated
	};

	enum class Button
	{
		Left,
		Right,
		Middle
	};

public:
	MouseEvent() = delete;

	MouseEvent(Type type, Button button) noexcept
		: type(type), button(button)
	{}

	bool IsPressed() const noexcept { return type == Type::Pressed; }
	bool IsReleased() const noexcept { return type == Type::Released; }
	bool IsRepeated() const noexcept { return type == Type::Repeated; }

	MouseEvent::Button GetButton() const noexcept { return button; }

private:
	Type type;
	Button button;
};
