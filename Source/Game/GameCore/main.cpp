// DirectX12EngineProject.cpp : 애플리케이션에 대한 진입점을 정의합니다.

#include <Windows.h>
#include "Utils/Utils.h"
#include <memory>
#include "MyGame.h"
#include "Utils/Window.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	Debug::CreateConsole();
#endif // DEBUG

	UINT width = 1280;
	UINT height = 720;

	Window gameWindow(hInstance, L"My DirectX 12 Engine", width, height);
	if (!gameWindow.InitInstance(nCmdShow))
	{
		Debug::Print(L"Window Creation Failed!");
		return -1;
	}

	MyGame gameApp = MyGame(hInstance);

	gameWindow.SetApp(&gameApp);
	if (!gameApp.Init(gameWindow.GetHWND(), width, height))
	{
		Debug::Print(L"Application Init Failed!");
		return -1;
	}

	gameWindow.Run();

	gameApp.Shutdown();

	return 0;
}