#pragma once

#include <windows.h>
#include <wrl.h>
#include <memory>
#include <vector>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

constexpr UINT FRAME_COUNT = 2; // Double buffering

// D3D12App은 이제 '엔진 프레임워크'의 추상 기반 클래스입니다.
// 이 클래스는 D3D12의 핵심 객체들(Device, Queue, SwapChain)을
// 소유하고, 메인 루프(Run)를 제공합니다.
class D3D12App
{
public:
	// HWND는 InitBase()에서 전달받아 초기화합니다.
	D3D12App(HINSTANCE hInstance);
	D3D12App(const D3D12App&) = delete;
	D3D12App& operator=(const D3D12App&) = delete;

	//상속을 위해 가상 소멸자는 필수입니다.
	virtual ~D3D12App();

	// InitBase()와 InitGame()을 '하나'의 Init()으로 통합합니다.
		// Game.exe의 MyGame::Init()이 내부에서 엔진의 InitBase()를 호출할 것입니다.
	virtual bool Init(HWND hWnd, UINT width, UINT height) = 0;

	// 메인 루프에서 호출될 '업데이트'
	virtual void Update(float dt) = 0;

	// 메인 루프에서 호출될 '렌더링'
	// (이전의 DrawGame()이 Render()가 됩니다.)
	void Render();

	// 윈도우에서 호출될 '리사이즈'
	void Resize(UINT width, UINT height);

	// 프로그램 종료 시
	virtual void Shutdown() = 0;

protected:
	// [게임이 구현할 '약속' (순수 가상 함수)]
	// 이 함수들은 '엔진'에 구현이 없습니다.
	// 'Game.exe'의 자식 클래스가 반드시 오버라이드해야 합니다.

	// 1. 게임에 특화된 리소스(PSO, 루트 시그니처, 메쉬)를 로드합니다.
	bool InitBase(HWND hWnd, UINT width, UINT height);
	// 2. 그리기 (커맨드 리스트 채우기)
	virtual void DrawGame() = 0;
	// 3. 창 크기 변경 시
	virtual void OnResize(UINT width, UINT height) = 0;


	// [엔진 유틸리티]
	// 자식 클래스가 사용할 수 있도록 protected로 변경합니다.
	void WaitForGPU();
	void MoveToNextFrame();
	void SetViewportAndScissorRect(UINT width, UINT height);

	// ImGui는 디버그 툴이므로 '엔진'이 소유하는 것이 합리적입니다.
	std::unique_ptr<class ImGuiManager> m_imguiManager;

	// 모든 D3D 객체와 멤버 변수들은 자식 클래스(MyGame)가 
	// 접근해야 하므로 'protected'로 변경합니다.
protected:
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargetBuffers[FRAME_COUNT];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	// ImGui 힙은 엔진이 관리합니다.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_imguiDescHeap;

	UINT m_rtvDescriptorSize;
	UINT m_frameIndex;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	// 게임 루프를 위한 타이머 추가
	std::unique_ptr<class Timer> m_pTimer;
};