#pragma once

#include "Event.h"
#include "KeyboardEvents.h"
#include "WindowEvents.h"

class EventHandler
{
public:

	static bool Dispatch(Event& event);

private:

	static EventHandler& Get()
	{
		static EventHandler handler;
		return handler;
	}

};