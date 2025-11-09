#include "D3D12App.h"
#include "Utils/Utils.h" // ThrowIfFailed, Debug::Print 등
#include "Utils/Timer.h"  // Timer 클래스
#include "Managers/ImGuiManager.h" // ImGui
#include "D3DX12/d3dx12.h"
#include "Components/GameObject.h"
#include "Components/Camera.h"

constexpr float CLEAR_COLOR[] = { 0.0f, 0.2f, 0.4f, 1.0f }; // Clear Color RGBA

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

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
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

#if defined(_EDITOR_MODE)
	// ImGui 초기화
	m_imguiManager = std::make_unique<ImGuiManager>
		(m_device.Get(), m_commandQueue.Get(), hWnd, FRAME_COUNT);

	// 에디터 카메라 생성
	m_editorCameraObject = std::make_unique<GameObject>("Editor Camera");
	m_editorCameraObject->GetTransform()->SetPosition(0.0f, 5.0f, -10.0f);
	m_editorCamera = m_editorCameraObject->AddComponent<Camera>();
	m_editorCamera->Init();

	CreateRttResources();

	m_editorViewImGuiHandle = m_imguiManager->RegisterTexture(m_editorTexture.Get());
	m_gameViewImGuiHandle = m_imguiManager->RegisterTexture(m_gameTexture.Get());

#endif // _EDITOR_MODE

	return true;
}

void D3D12App::Render()
{
	// 1. [엔진] 프레임 준비
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));


	auto barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargetBuffers[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	m_commandList->ResourceBarrier(1, &barrierToRenderTarget);

#if defined(_EDITOR_MODE)
	// ImGui
	m_imguiManager->NewFrame();

	// Set ImGui Window Flags
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// Fullscreen ImGui Window
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	// Begin DockSpace
	ImGui::Begin("DockSpace Demo", nullptr, window_flags);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("New Project");
			ImGui::MenuItem("Save Project");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("About My Engine");
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGuiID dockspace_id = ImGui::GetID("MyWindow");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	ImGui::PopStyleVar(3);

	// Render your ImGui elements here	
	ImGui::Begin("Inspector");
	ImGui::End();

	ImGui::Begin("Scene");
	{
		//Scene Viewport Rendering
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		if (viewportSize.x > 0 && viewportSize.y > 0)
		{
			// Store the viewport size for later use
			m_editorViewportSize = viewportSize;

			if (m_editorCamera)
			{
				m_editorCamera->SetProjectionMatrix(
					60.0f,
					m_editorViewportSize.x / m_editorViewportSize.y,
					0.1f,
					1000.0f);
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Game");
	{
		//Game Viewport Rendering
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		if (viewportSize.x > 0 && viewportSize.y > 0)
		{
			// Store the viewport size for later use
			m_gameViewportSize = viewportSize;

			if (m_gameCamera)
			{
				m_gameCamera->SetProjectionMatrix(
					60.0f,
					m_gameViewportSize.x / m_gameViewportSize.y,
					0.1f,
					1000.0f);
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Hierarchy");
	ImGui::End();

	// Project
	ImGui::Begin("Project");
	ImGui::End();

	ImGui::Begin("Console");
	ImGui::End();

	if (m_editorCamera)
	{
		D3D12_RESOURCE_BARRIER barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
			m_editorTexture.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_commandList->ResourceBarrier(1, &barrierToRenderTarget);

		m_commandList->OMSetRenderTargets(1,
			&m_editorRtvHandle,
			FALSE,
			&m_editorDsvHandle);

		D3D12_VIEWPORT editorViewport = {0, 0,
			m_editorViewportSize.x,
			m_editorViewportSize.y,
			0.0f, 1.0f };

		D3D12_RECT editorScissor = {0, 0,
			static_cast<LONG>(m_editorViewportSize.x),
			static_cast<LONG>(m_editorViewportSize.y)};

		m_commandList->RSSetViewports(1, &editorViewport);
		m_commandList->RSSetScissorRects(1, &editorScissor);

		m_commandList->ClearRenderTargetView(
			m_editorRtvHandle,
			CLEAR_COLOR,
			0, nullptr);

		m_commandList->ClearDepthStencilView(
			m_editorDsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, 0,
			0, nullptr);

		Render3DScene(*m_editorCamera);

		D3D12_RESOURCE_BARRIER barrierToShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
			m_editorTexture.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_commandList->ResourceBarrier(1, &barrierToShaderResource);
	}

	if (m_gameCamera)
	{
		D3D12_RESOURCE_BARRIER barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
			m_gameTexture.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_commandList->ResourceBarrier(1, &barrierToRenderTarget);
		
		m_commandList->OMSetRenderTargets(1,
			&m_gameRtvHandle,
			FALSE,
			&m_gameDsvHandle);
		
		D3D12_VIEWPORT gameViewport = { 0, 0,
			m_gameViewportSize.x,
			m_gameViewportSize.y,
			0.0f, 1.0f };
		
		D3D12_RECT gameScissor = { 0, 0,
			static_cast<LONG>(m_gameViewportSize.x),
			static_cast<LONG>(m_gameViewportSize.y) };
		
		m_commandList->RSSetViewports(1, &gameViewport);
		m_commandList->RSSetScissorRects(1, &gameScissor);
		
		m_commandList->ClearRenderTargetView(
			m_gameRtvHandle,
			CLEAR_COLOR,
			0, nullptr);
		
		m_commandList->ClearDepthStencilView(
			m_gameDsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, 0,
			0, nullptr);
		
		Render3DScene(*m_gameCamera);

		D3D12_RESOURCE_BARRIER barrierToShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
			m_gameTexture.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_commandList->ResourceBarrier(1, &barrierToShaderResource);
	}

	{
		auto rtvHandle = GetCurrentBackBufferRtv();
		auto dsvHandle = GetCurrentDsv();
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Set viewport and scissor rect
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Clear the render target and depth stencil
		m_commandList->ClearRenderTargetView(rtvHandle, CLEAR_COLOR, 0, nullptr);
		m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		ImGui::Begin("Scene");
		ImGui::Image(m_editorViewImGuiHandle, m_editorViewportSize);
		ImGui::End();
		ImGui::Begin("Game");
		ImGui::Image(m_gameViewImGuiHandle, m_gameViewportSize);
		ImGui::End();

		// Render the ImGui Scene Viewport
		m_imguiManager->Render(m_commandList.Get());
	}

#else

	auto rtvHandle = GetCurrentBackBufferRtv();
	auto dsvHandle = GetCurrentDsv();

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Clear the render target and depth stencil
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	if (m_gameCamera)
	{
		Render3DScene(*m_gameCamera);
	}

#endif // _EDITOR_MODE

	// 프레임 제출
	// 리소스 상태 전환 (Render Target -> Present)
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
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
	const UINT64 fence = ++m_fenceValue; // 다음 값
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

// GPU가 모든 명령을 처리할 때까지 기다린다
void D3D12App::WaitForGPU()
{
	const UINT64 fence = ++m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
	WaitForSingleObject(m_fenceEvent, INFINITE);
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

	// RTV 생성 설명자 설정 (ImGUI용 SRGB)
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargetBuffers[n]));
		m_device->CreateRenderTargetView(m_renderTargetBuffers[n].Get(), &rtvDesc, rtvHandle);
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

void D3D12App::CreateRttResources()
{
#if defined(_EDITOR_MODE)
	// Render To Texture용 뷰 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2; // Scene View용 1개 + Game View 1개
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rttRtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 2; // Scene View용 1개 + Game View 1개
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_rttDsvHeap)));

	// RTT 리소스의 기본 클리어 색상 설정
	D3D12_CLEAR_VALUE rtvClearValue = {};
	rtvClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	std::copy(std::begin(CLEAR_COLOR), std::end(CLEAR_COLOR), rtvClearValue.Color); // RGBA 복사

	D3D12_CLEAR_VALUE dsvClearValue = {};
	dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;

	// 힙 핸들 주소 계산
	UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_editorRtvHandle = m_rttRtvHeap->GetCPUDescriptorHandleForHeapStart(); // 0번 슬롯
	m_editorDsvHandle = m_rttDsvHeap->GetCPUDescriptorHandleForHeapStart(); // 0번 슬롯
	
	m_gameRtvHandle = m_editorRtvHandle; // 0번 복사
	m_gameRtvHandle.ptr += rtvDescriptorSize; // 1칸 이동

	m_gameDsvHandle = m_editorDsvHandle; // 0번 복사
	m_gameDsvHandle.ptr += dsvDescriptorSize; // 1칸 이동

	// Scene View용 RTT 텍스처 생성
	// D3D12_RESOURCE_DESC 설정
	auto sceneTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // 포맷
		static_cast<UINT64>(m_editorViewportSize.x), //(), // 임시 크기
		static_cast<UINT>(m_editorViewportSize.y),
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);

	// CreateCommittedResource 호출
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&sceneTexDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	// ImGui가 읽을 수 있도록 초기 상태
		&rtvClearValue,
		IID_PPV_ARGS(&m_editorTexture)
	));

	m_device->CreateRenderTargetView(m_editorTexture.Get(), nullptr, m_editorRtvHandle);

	// Scene 깊이 버퍼 생성
	auto sceneDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,	// 깊이 포맷
		static_cast<UINT64>(m_editorViewportSize.x),
		static_cast<UINT>(m_editorViewportSize.y),
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL // 깊이 스텐실 허용
	);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&sceneDepthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 깊이 쓰기 상태로 시작
		&dsvClearValue,
		IID_PPV_ARGS(&m_editorDepthBuffer)
	));

	m_device->CreateDepthStencilView(m_editorDepthBuffer.Get(), nullptr, m_editorDsvHandle);

	// Game View용 RTT 텍스처 생성
	auto gameTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // 포맷
		static_cast<UINT64>(m_gameViewportSize.x), // 임시 크기
		static_cast<UINT>(m_gameViewportSize.y),
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&gameTexDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	// ImGui가 읽을 수 있도록 초기 상태
		&rtvClearValue,
		IID_PPV_ARGS(&m_gameTexture)
	));
	m_device->CreateRenderTargetView(m_gameTexture.Get(), nullptr, m_gameRtvHandle);
	
	// Game 깊이 버퍼 생성
	auto gameDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,	// 깊이 포맷
		static_cast<UINT64>(m_gameViewportSize.x),
		static_cast<UINT>(m_gameViewportSize.y),
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL // 깊이 스텐실 허용
	);

	// CreateCommittedResource 호출
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&gameDepthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 깊이 쓰기 상태로 시작
		&dsvClearValue,
		IID_PPV_ARGS(&m_gameDepthBuffer)
	));
	m_device->CreateDepthStencilView(m_gameDepthBuffer.Get(), nullptr, m_gameDsvHandle);
#endif // _EDITOR_MODE
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::GetCurrentBackBufferRtv() const
{
	// 현재 프레임의 RTV 핸들 반환
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	
	// Offset by frame index
	rtvHandle.Offset(m_frameIndex, m_rtvDescriptorSize);
	
	// Return the handle
	return rtvHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::GetCurrentDsv() const
{
	return m_dsvHeap->GetCPUDescriptorHandleForHeapStart(); // DSV 힙에는 하나의 DSV만 존재
}
