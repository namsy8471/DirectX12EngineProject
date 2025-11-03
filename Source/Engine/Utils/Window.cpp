#include "Window.h"
#include "imgui_impl_win32.h"

#include "Core\D3D12App.h"
#include "Utils\Timer.h"

#include <stdexcept>

Window::Window(HINSTANCE hInstance, const WCHAR* title, UINT width, UINT height)
    : m_width(width),
    m_height(height),
    m_hInst(hInstance),
    m_title(title),
    m_className(L"MyEngineWindowClass")
{
    // [수정] 실패를 감지하도록 변경
    if (!MyRegisterClass(hInstance))
    {
        throw std::runtime_error("Failed to register window class.");
    }

	m_hInst = hInstance;
}

Window::~Window()
{
}


bool Window::MyRegisterClass(HINSTANCE hInstance) const
{
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Window::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = m_className.c_str();
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

BOOL Window::InitInstance(int nCmdShow)
{
    m_hWnd = CreateWindowW(
        m_className.c_str(),
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
        nullptr,
        nullptr,//LoadMenu(m_hInst, MAKEINTRESOURCE(IDC_DIRECTX12ENGINEPROJECT)),
        m_hInst,
        this);

    if (!m_hWnd)
    {
        throw std::runtime_error("Failed to initialize application instance.");
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return TRUE;
}

// 'main.cpp'가 GameApp을 연결해주는 함수
void Window::SetApp(D3D12App* pApp)
{
    m_d3dApp = pApp;
}

// 메인 루프 수정 (가장 중요)
void Window::Run()
{
    MSG msg = {};

    // 타이머는 D3D12App이 아닌 '메인 루프'가 소유합니다.
    Timer timer;
    timer.Reset();

    // 기본 메시지 루프입니다:
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
        }
        else
        {
            // m_d3dApp->Render()만 호출하는 것이 아니라,
            // Update와 Render를 모두 호출해야 합니다.
            if (m_d3dApp)
            {
                timer.Tick();
                m_d3dApp->Update(timer.GetDeltaTime());
                m_d3dApp->Render();
            }
        }
    }
}

LRESULT Window::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        if (m_d3dApp)
        {
            m_width = LOWORD(lParam);
            m_height = HIWORD(lParam);
            m_d3dApp->Resize(m_width, m_height);
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

// ImGui 윈도우 프로시저 핸들러 선언 (현재 imgui_impl_win32.h에 extern으로 선언되어 있음)
// extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* app = nullptr;

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

    if (message == WM_NCCREATE)
    {
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		app = reinterpret_cast<Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
		app = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
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