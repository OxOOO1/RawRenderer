#define _CRT_SECURE_NO_WARNINGS
#include "Application.h"

#include <sstream>
#include <iomanip>

#include "Utilities/FileIO.h"

#include "Input/Mouse.h"
#include "Input/Keyboard.h"


SApplication::SApplication(unsigned int windowWidth, unsigned int windowHeight, std::string name)
	: 
	Window(windowWidth, windowHeight, name.c_str()),
	LLRenderer(Window.GetHandle(), { windowWidth, windowHeight })
{
	assert(sInstance == nullptr);
	sInstance = this;

	SystemTime::Initialize();
	RGpuTimeManager::Initialize();

	ImGuiManager = std::make_unique<SImGuiManager>(Window.GetHandle());

	const UINT RTWidth = windowWidth;
	const UINT RTHeight = windowHeight;

	Renderer = std::make_unique<SRenderer>(UINT2{ RTWidth, RTHeight });

	Mouse::Get().CreateDIDevice(Window.GetHandle());

}

bool SApplication::OnEvent(Event& event)
{
	
	Window.OnEvent(event);
	//Scene->OnEvent(event);

	return event.Handled;

}

int SApplication::Run()
{
	std::optional<int> exitcode{ std::nullopt };
	while (!exitcode)
	{

		exitcode = SWindow::ProcessMessage();

		DoFrame();

	}

	Renderer.reset();
	LLRenderer.Terminate();
	LLRenderer.Shutdown();

	return *exitcode;
	
}

void SApplication::DoFrame()
{
	ImGuiManager->NewFrame();

	Mouse::Get().OnUpdate(Window.bCursorEnabled);
	
	Time::Get().OnUpdate();

	Renderer->Render(LLRenderer);

	Mouse::Get().FlushPosOffset();

	RProfileEvent::EndFrame();

}

SApplication* SApplication::sInstance;
