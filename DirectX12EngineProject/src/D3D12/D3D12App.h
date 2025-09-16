#pragma once

#include <windows.h>
#include <wrl.h>

class D3D12App
{
public:
	D3D12App(HWND hWnd);
	~D3D12App();
	bool Initialize();
	void Render();
	void Resize(UINT width, UINT height);

private:
	Microsoft::WRL::ComPtr<struct IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<struct ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<struct ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<struct ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<struct ID3D12GraphicsCommandList> m_commandList;

	Microsoft::WRL::ComPtr<struct IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<struct ID3D12Resource> m_renderTargets[2];

	UINT m_rtvDescriptorSize;
	UINT m_frameIndex;
};

