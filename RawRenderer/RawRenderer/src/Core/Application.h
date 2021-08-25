#pragma once
#include "Window.h"

#include "Utilities/Time.h"
#include "Utilities/ImGuiManager.h"

#include "Events/Event.h"

#include "Scene/Scene.h"
#include "Renderer.h"

#include "LLRenderer.h"

class SApplication
{
public:
	
	SApplication(unsigned int windowWidth = 1024,unsigned int windowHeight = 768, std::string name = "Default Application");
	SApplication() = delete;
	SApplication(const SApplication&) = delete;
	SApplication& operator=(const SApplication&) = delete;
	SApplication(const SApplication&&) = delete;
	SApplication& operator=(const SApplication&&) = delete;

	int Run();
	bool OnEvent(Event& event);

	static SApplication& Get()
	{
		return *sInstance;
	}
private:
	void DoFrame();

private:
	SWindow Window;

	SLLRenderer LLRenderer;

	std::unique_ptr<SRenderer> Renderer;

	std::unique_ptr<SImGuiManager> ImGuiManager;

	RGpuTimer GpuTimer;

private:
	static SApplication* sInstance;

};