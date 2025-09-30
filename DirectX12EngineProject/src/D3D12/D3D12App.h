#pragma once

#include <windows.h>
#include <wrl.h>
#include <memory>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

constexpr UINT FRAME_COUNT = 2; // Double buffering

class D3D12App
{
public:
	D3D12App(HWND hWnd, UINT width, UINT height);
	~D3D12App();

	void Render();
	void Resize(UINT width, UINT height);

private:
	void WaitForGPU();
	void MoveToNextFrame();
	void SetViewportAndScissorRect(UINT width, UINT height);

#ifdef _DEBUG
	std::unique_ptr<class ImGuiManager> m_imguiManager;
#endif // _DEBUG

	Microsoft::WRL::ComPtr<struct IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<struct ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<struct ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<struct ID3D12CommandAllocator> m_commandAllocators[FRAME_COUNT];
	Microsoft::WRL::ComPtr<struct ID3D12GraphicsCommandList> m_commandList;

	Microsoft::WRL::ComPtr<struct IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<struct ID3D12Resource> m_renderTargetBuffers[FRAME_COUNT];

	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<struct ID3D12Resource> m_depthStencilBuffer;

	Microsoft::WRL::ComPtr<struct ID3D12DescriptorHeap> m_imguiDescHeap;

	UINT m_rtvDescriptorSize;
	UINT m_frameIndex;

	Microsoft::WRL::ComPtr<struct ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<struct ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<struct ID3D12PipelineState> m_pso;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
};

