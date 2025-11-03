#include "D3D12App.h"
#include "Utils/Utils.h" // ThrowIfFailed, Debug::Print 등
#include "Utils/Timer.h"  // Timer 클래스
#include "Managers/ImGuiManager.h" // ImGui
#include "D3DX12/d3dx12.h"

// 생성자: 멤버 변수 초기화
D3D12App::D3D12App(HINSTANCE hInstance)
	: m_hInstance(hInstance),
	m_hWnd(nullptr),
	m_rtvDescriptorSize(0),
	m_frameIndex(0),
	m_fenceValue(0),
	m_fenceEvent(nullptr),
	m_pTimer(nullptr)
{
	m_viewport = {};
	m_scissorRect = {};
}

// 소멸자: GPU 동기화 및 핸들 해제
D3D12App::~D3D12App()
{
	// Ensure that the GPU is no longer referencing resources
	WaitForGPU();
	CloseHandle(m_fenceEvent);
}

// '엔진'의 모든 공통 객체를 초기화합니다.
bool D3D12App::InitBase(HWND hWnd, UINT width, UINT height)
{
	m_hWnd = hWnd;
	using namespace Microsoft::WRL;

	// 원본 생성자의 모든 코드를 InitBase()로 이동
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
	// ... (GPUBasedValidation 등)
#endif // DEBUG

	// Create DXGI Factory
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory)));

	// Create D3D12 Device
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_COUNT;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(m_factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));
	ThrowIfFailed(swapChain.As(&m_swapChain));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Fence & Event
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValue = 0;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// Create RTV Heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FRAME_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create DSV Heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
	}

	// RTV/DSV 생성은 Resize() 함수가 처리하도록 위임합니다.
	// Resize는 RTV와 DSV를 (재)생성합니다.
	Resize(width, height);

	// Create Command Allocators
	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		ThrowIfFailed(m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocators[n])
		));
	}

	// Create Command List
	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[m_frameIndex].Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	));
	m_commandList->Close(); // 즉시 닫습니다. Render()에서 Reset()할 것입니다.

	// ImGui 초기화 (엔진이 담당)
	m_imguiManager = std::make_unique<ImGuiManager>
		(m_device.Get(), m_commandQueue.Get(), hWnd, FRAME_COUNT);

	return true;
}

// Render()는 이제 '템플릿 메서드'입니다.
// 1. 프레임 준비 (엔진)
// 2. 게임 그리기 (자식 클래스 가상 함수 호출)
// 3. ImGui 그리기 (엔진)
// 4. 프레임 제출 (엔진)
void D3D12App::Render()
{
	// 1. [엔진] 프레임 준비
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	// 뷰포트와 시저렉트 설정
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// 리소스 상태 전환 (Present -> Render Target)
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargetBuffers[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_commandList->ResourceBarrier(1, &barrier);

	// Render Target 설정
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_imguiManager->NewFrame();

	// 2. [게임] 그리기 (가상 함수 호출)
	// 자식 클래스(MyGame)가 PSO, RootSig, VB/IB를 설정하고
	// DrawIndexedInstanced()를 호출하는 부분이 여기 들어옵니다.
	DrawGame();


	// 3. [엔진] ImGui 그리기
	m_imguiManager->Render(m_commandList.Get());

	// 4. [엔진] 프레임 제출
	// 리소스 상태 전환 (Render Target -> Present)
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargetBuffers[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_commandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// 스왑 체인 Present 및 다음 프레임으로 이동
	ThrowIfFailed(m_swapChain->Present(1, 0)); // VSync
	MoveToNextFrame();
}

void D3D12App::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_fenceValue; // 다음 값
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

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
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
	WaitForSingleObject(m_fenceEvent, INFINITE);
	m_fenceValue++;
}

// Resize 함수를 수정하여 '깊이 버퍼(DSV)'도 재생성하도록 버그를 수정합니다.
void D3D12App::Resize(UINT width, UINT height)
{
	if (width == 0 || height == 0 || (width == m_viewport.Width && height == m_viewport.Height))
		return;

	WaitForGPU();

	// RTV 버퍼 해제
	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		m_renderTargetBuffers[n].Reset();
	}
	// DSV 버퍼도 반드시 해제해야 합니다.
	m_depthStencilBuffer.Reset();

	// 스왑 체인 버퍼 크기 변경
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		FRAME_COUNT,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0
	));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// RTV 재생성
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargetBuffers[n]));
		m_device->CreateRenderTargetView(m_renderTargetBuffers[n].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}

	// DSV 재생성
	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = DXGI_FORMAT_D32_FLOAT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(&m_depthStencilBuffer)));

	// DSV 힙에 DSV 생성
	m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// 뷰포트 및 시저 렉트 업데이트
	SetViewportAndScissorRect(width, height);

	// [가상 함수 호출] '게임'에게 리사이즈를 알림
	OnResize(width, height);
}

void D3D12App::SetViewportAndScissorRect(UINT width, UINT height)
{
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = width;
	m_scissorRect.bottom = height;
}