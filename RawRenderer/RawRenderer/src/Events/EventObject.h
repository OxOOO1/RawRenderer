#pragma once

#include "Events/Event.h"
#include "Events/WindowEvents.h"

class IEventObject
{
public:

	virtual void OnEvent(Event& event) = 0;
};