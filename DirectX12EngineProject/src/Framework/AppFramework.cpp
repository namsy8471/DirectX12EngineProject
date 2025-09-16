#include "AppFramework.h"

#include "..\Core\framework.h"
#include "..\Core\DirectX12EngineProject.h"
#include "..\D3D12\D3D12App.h"

#include <stdexcept>

AppFramework::AppFramework(HINSTANCE hInstance, int nCmdShow)
{
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DIRECTX12ENGINEPROJECT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	if(!InitInstance(hInstance, nCmdShow))
    {
        throw std::runtime_error("Failed to initialize application instance.");
	}

	m_d3dApp = std::make_unique<D3D12App>(m_hWnd);
}

AppFramework::~AppFramework()
{
}

void AppFramework::Run()
{
    HACCEL hAccelTable = LoadAccelerators(m_hInst, MAKEINTRESOURCE(IDC_DIRECTX12ENGINEPROJECT));

    MSG msg = {};

    // 기본 메시지 루프입니다:
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
			m_d3dApp->Render();
        }
    }
}

LRESULT AppFramework::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 메뉴 선택을 구문 분석합니다:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AppFramework::About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        //PAINTSTRUCT ps;
        //HDC hdc = BeginPaint(hWnd, &ps);
        //// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
        //EndPaint(hWnd, &ps);

        ValidateRect(hWnd, NULL);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK AppFramework::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	AppFramework* app = nullptr;

    if (message == WM_NCCREATE)
    {
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		app = reinterpret_cast<AppFramework*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
		app = reinterpret_cast<AppFramework*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if(app)
    {
        return app->HandleMessage(hWnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

BOOL AppFramework::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    m_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr,
        LoadMenu(m_hInst, MAKEINTRESOURCE(IDC_DIRECTX12ENGINEPROJECT)), 
        hInstance, 
        this);

    if (!m_hWnd)
    {
        return FALSE;
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return TRUE;
}

bool AppFramework::MyRegisterClass(HINSTANCE hInstance) const
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = AppFramework::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTX12ENGINEPROJECT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DIRECTX12ENGINEPROJECT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK AppFramework::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}