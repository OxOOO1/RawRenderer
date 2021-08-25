#pragma once
#include <queue>

#define PasteTypeCheckMethods(type)  static EventType GetType() {return EventType::##type; }\
									 virtual EventType GetEventType() const override {return GetType(); }\
									 virtual bool CheckTypeAndHandle(EventType type) override { if (GetType() == type) { Handled = true; return true; } else return false; }



enum class EventType
{
	None = 0,
	KeyboardEvent, KeyboardCharEvent,
	MouseCursorEvent,
	MouseEvent

};

enum EventCategory
{
	None = 0,
	EventCategoryApplication = 1 << 0,
	EventCategoryInput = 1 << 1,
	EventCategoryKeyboard = 1 << 2,
	EventCategoryMouse = 1 << 3,
	EventCategoryMouseButton = 1 << 4
};

class Event
{
public:

	virtual EventType GetEventType() const = 0;
	virtual bool CheckTypeAndHandle(EventType type) = 0;

	static EventType GetType();

	bool Handled = false;

};
