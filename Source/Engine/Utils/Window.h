#pragma once

#include <windows.h>
#include <memory>
#include <string>

const int MAX_LOADSTRING = 100;
class D3D12App;

class Window
{
public:
	Window(HINSTANCE hInstance, const WCHAR* title, UINT width, UINT height);
	~Window();

	void Run();

	// InitInstance()를 public으로 변경하여 main에서 호출
	BOOL InitInstance(int nCmdShow);
	HWND GetHWND() const { return m_hWnd; } // HWND를 반환하는 getter

	// 'main.cpp'가 Window에게 GameApp을 알려줄 수 있는 'Setter'
	void SetApp(D3D12App* pApp);

private:
	HINSTANCE m_hInst;                                
	HWND m_hWnd;
	std::wstring m_title;                  // 제목 표시줄 텍스트입니다.
	std::wstring m_className;              // 기본 창 클래스 이름입니다.

	UINT m_width;
	UINT m_height;

	D3D12App* m_d3dApp;

	bool MyRegisterClass(HINSTANCE hInstance) const;

	LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); // 메시지 처리 함수
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

