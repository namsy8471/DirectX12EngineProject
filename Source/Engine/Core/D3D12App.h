#pragma once

#include <windows.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include "imgui.h" // ImGui::ImTextureID

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

constexpr UINT FRAME_COUNT = 2; // Double buffering

class GameObject;
class Camera;

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
	virtual void Render3DScene(const Camera& camera) = 0;
	// 3. 창 크기 변경 시
	virtual void OnResize(UINT width, UINT height) = 0;

	// [엔진 유틸리티]
	// 자식 클래스가 사용할 수 있도록 protected로 변경합니다.
	void WaitForGPU();
	void MoveToNextFrame();
	void SetViewportAndScissorRect(UINT width, UINT height);

	// RTT(Render To Texture) 관련 함수
	void CreateRttResources();
	
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	// D3D12 핵심 객체들
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

	// 렌더 타겟(RTV) 관련
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargetBuffers[FRAME_COUNT];
	UINT m_rtvDescriptorSize; // RTV 디스크립터 크기

	// 깊이 버퍼(DSV) 관련
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	// 현재 프레임 인덱스
	UINT m_frameIndex;

	// GPU 동기화용 펜스
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	// 뷰포트와 시저 렉트
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	// 게임 루프를 위한 타이머 추가
	std::unique_ptr<class Timer> m_pTimer;

	// Editor 전용 멤버들
#if defined(_EDITOR_MODE)

	// ImGui
	std::unique_ptr<class ImGuiManager> m_imguiManager;

	// Editor Camera
	std::unique_ptr<GameObject> m_editorCameraObject;
	Camera* m_editorCamera = nullptr;

	// Render To Texture용 리소스와 뷰 힙
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rttRtvHeap;	// 렌더 타겟 뷰 힙
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rttDsvHeap;  // 깊이 스텐실 뷰 힙

	// Scene View RTT 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> m_editorTexture; // Scene 렌더 타겟 텍스처
	Microsoft::WRL::ComPtr<ID3D12Resource> m_editorDepthBuffer; // Scene 깊이 버퍼 텍스처
	D3D12_CPU_DESCRIPTOR_HANDLE m_editorRtvHandle; // RTT RTV 힙의 0번 슬롯
	D3D12_CPU_DESCRIPTOR_HANDLE m_editorDsvHandle; // RTT DSV 힙의 0번 슬롯
	ImTextureID m_editorViewImGuiHandle = 0; // ImGui용 텍스처 핸들
	ImVec2 m_editorViewportSize = { 1280, 720 }; // ImGui에 표시할 때의 크기

	// Game View RTT 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> m_gameTexture; // Scene 렌더 타겟 텍스처
	Microsoft::WRL::ComPtr<ID3D12Resource> m_gameDepthBuffer; // Scene 깊이 버퍼 텍스처
	D3D12_CPU_DESCRIPTOR_HANDLE m_gameRtvHandle; // RTT RTV 힙의 1번 슬롯
	D3D12_CPU_DESCRIPTOR_HANDLE m_gameDsvHandle; // RTT DSV 힙의 1번 슬롯
	ImTextureID m_gameViewImGuiHandle = 0; // ImGui용 텍스처 핸들
	Camera* m_gameCamera;	// Game View에 사용할 카메라 (게임이 할당해줘야 함)
	ImVec2 m_gameViewportSize = { 1280, 720 }; // ImGui에 표시할 때의 크기
#endif // _EDITOR_MODE

private:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtv() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDsv() const;
};