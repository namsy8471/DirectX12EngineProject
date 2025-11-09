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
	void Render(ID3D12GraphicsCommandList* cmdList);

	ImTextureID RegisterTexture(ID3D12Resource* pTextureResource);

private:
	// Singleton for callback functions without Capture
	static ImGuiManager* s_instance;
	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_imguiDescHeap;
	DescriptorHeapAllocator m_imGuiDescHeapAlloc;

	ID3D12Device* m_device = nullptr; // D3D12 디바이스
	const int m_totalHeapSize = 128; // 총 디스크립터 개수
	UINT m_srvHandleIncrementSize = 0; // SRV 디스크립터 크기
	int m_currentHeapIndex = 1; // 현재 할당된 디스크립터 인덱스(0번은 항상 폰트이므로 1번 시작)
};
