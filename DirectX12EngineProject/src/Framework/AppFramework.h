#pragma once

#include <windows.h>
#include <memory>

const int MAX_LOADSTRING = 100;

class AppFramework
{
public:
	AppFramework(HINSTANCE hInstance, int nCmdShow);
	~AppFramework();
	void Run();

private:
	HINSTANCE m_hInst;                                
	HWND m_hWnd;
	WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
	WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

	std::unique_ptr<class D3D12App> m_d3dApp;

	bool MyRegisterClass(HINSTANCE hInstance) const;
	BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);

	LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); // 메시지 처리 함수
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

