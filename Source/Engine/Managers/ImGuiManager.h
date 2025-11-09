#pragma once

#include <windows.h>
#include <wrl.h>
#include "../Utils/DescriptorHeapAllocator.h"

class ImGuiManager
{
public:
	ImGuiManager(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, HWND hWnd, const int FRAME_COUNT);
	~ImGuiManager();
	void NewFrame();
	void Render(ID3D12GraphicsCommandList* cmdList, ImVec2& sceneViewport, ImVec2& gameViewport, Camera* sceneCamera, Camera* gameCamera);

	D3D12_VIEWPORT GetSceneViewportSize();
	D3D12_VIEWPORT GetGameViewportSize();

private:

	D3D12_GPU_DESCRIPTOR_HANDLE RegisterTexture(ID3D12Device* device, ID3D12Resource* textureResource);

	// Singleton for callback functions without Capture
	static ImGuiManager* s_instance;

	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_imguiDescHeap;
	DescriptorHeapAllocator m_ImGuiDescHeapAlloc;

	const int m_totalHeapSize = 128; // 총 디스크립터 개수
	UINT m_srvHandleIncrementSize = 0; // SRV 디스크립터 크기
	int m_currentHeapIndex = 1; // 현재 할당된 디스크립터 인덱스(0번은 항상 폰트이므로 1번 시작)
};
