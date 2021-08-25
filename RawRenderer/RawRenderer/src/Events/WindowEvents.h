#pragma once

#include "Event.h"

class MouseCursorEvent : public Event
{
public:

	PasteTypeCheckMethods(MouseCursorEvent)

	enum class CursorState
	{
		Shown = 0,
		Hidden
	};

	MouseCursorEvent(CursorState newState)
		:state(newState) {}

	CursorState state;

};
