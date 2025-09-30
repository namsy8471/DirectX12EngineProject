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

private:
	// Singleton for callback functions without Capture
	static ImGuiManager* s_instance;

	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_imguiDescHeap;
	DescriptorHeapAllocator m_ImGuiDescHeapAlloc;
};
