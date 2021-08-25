#include "framework.h"

#include "Core\Application.h"
#include "Core\Window.h"

constexpr auto ScreenWidth = 1280;
constexpr auto ScreenHeight = 720;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hprevInstance, LPSTR pCmdLine, int nCmdShow)
{
	Time::Get();

	SApplication application(ScreenWidth, ScreenHeight);
	return application.Run();

	//return -1;
	//return (WPARAM)message.wParam; 

}