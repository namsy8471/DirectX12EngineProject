// DirectX12EngineProject.cpp : 애플리케이션에 대한 진입점을 정의합니다.

#include "../Framework/AppFramework.h"
#include "../Utils/Utils.h"
#include <memory>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
    CreateConsole();
#endif // DEBUG

	std::unique_ptr<AppFramework> pApp = 
        std::make_unique<AppFramework>(hInstance, nCmdShow);
    
    pApp->Run();

    return 0;
}