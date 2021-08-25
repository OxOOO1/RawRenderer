#include "EventHandler.h"

#include "Core/Application.h"

bool EventHandler::Dispatch(Event & event)
{
	return SApplication::Get().OnEvent(event);
}
