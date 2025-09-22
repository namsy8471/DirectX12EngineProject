#include "D3D12App.h"

#include "../Utils/Utils.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

D3D12App::D3D12App(HWND hWnd)
{
	using namespace Microsoft::WRL;

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))
	{
		debugController->EnableDebugLayer();
	}

	Debug::Print(L"Debug Controller Load Complete!");

	ComPtr<ID3D12Debug1> debugController1;
	ThrowIfFailed(debugController.As(&debugController1))
	{
		debugController1->SetEnableGPUBasedValidation(TRUE);
	}


	Debug::Print(L"Debug Controller1 Load Complete!");
#endif // DEBUG

	// Create DXGI Factory
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)))
	{
		Debug::Print(L"Factory2 Load Complete!");
	}

	ThrowIfFailed(factory.As(&m_factory))
	{
		Debug::Print(L"Factory4 Load Complete!");
	}

	// Create D3D12 Device
	ThrowIfFailed(D3D12CreateDevice(
		nullptr, // default adapter
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&m_device)
	))
	{
		Debug::Print(L"D3D12 Device Load Complete!");
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // direct, compute, copy
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))
	{
		Debug::Print(L"Command Queue Load Complete!");
	}

	// Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2; // double buffering
	swapChainDesc.Width = 1280; // TODO: use window width
	swapChainDesc.Height = 720; // TODO: use window height
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // this is the most common swapchain format
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // all modern apps must use this SwapEffect
	swapChainDesc.SampleDesc.Count = 1; // don't use multi-sampling

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	))
	{
		Debug::Print(L"Swap Chain Load Complete!");
	}

	ThrowIfFailed(swapChain.As(&m_swapChain))
	{
		Debug::Print(L"Swap Chain3 Load Complete!");
	}
	// This sample does not support fullscreen transitions.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create Descriptor Heaps
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2; // double buffering
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for(UINT n = 0; n < 2; n++)
	{
		// 스왑 체인으로부터 n번째 버퍼 리소스를 가져옵니다.
		m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
		// 해당 버퍼에 대한 RTV를 생성하여 힙에 저장합니다.
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		// 다음 RTV를 저장할 위치로 핸들을 이동합니다.
		rtvHandle.ptr += m_rtvDescriptorSize;

		Debug::Print(L"Render Target View [" + std::to_wstring(n) + L"] Load Complete!");
	}

	// Create Command Allocators
	for(UINT n = 0; n < FrameCount; n++)
	{
		ThrowIfFailed(m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocators[n])
		))
		{
			Debug::Print(L"Command Allocator [" + std::to_wstring(n) + L"] Load Complete!");
		}
	}

	// Create Command List
	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[m_frameIndex].Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	))
	{
		Debug::Print(L"Command List Load Complete!");
	}

	m_commandList->Close();

	// Fence & Event
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))
	{
		Debug::Print(L"Fence Load Complete!");
	}

	m_fenceValue = 0;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

D3D12App::~D3D12App()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForGPU();
	CloseHandle(m_fenceEvent);
}

void D3D12App::Render()
{
	// 이 슬롯이 다시 돌아올 때까지 기다릴 값을 갱신한다
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	// Rendering commands
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	// 리소스 상태 전환
	m_commandList->ResourceBarrier(1, &barrier);

	// Render Target 설정
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;

	// Set the render target for the output merger stage (the output of the pipeline)
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
	m_commandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	MoveToNextFrame();
}

void D3D12App::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_fenceValue; // 다음 값
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	ThrowIfFailed(m_swapChain->Present(1, 0)); // VSync

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fence->GetCompletedValue() < m_fenceValue)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_fenceValue++;
}

// GPU가 모든 명령을 처리할 때까지 기다린다
void D3D12App::WaitForGPU()
{
	// 현재 값으로 신호를 보낸다
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

	// 이 값에 도달할 때까지 기다린다
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
	WaitForSingleObject(m_fenceEvent, INFINITE);
	
	// 다음 값을 위해 1 증가시킨다
	m_fenceValue++;
}


void D3D12App::Resize(UINT width, UINT height)
{
}
