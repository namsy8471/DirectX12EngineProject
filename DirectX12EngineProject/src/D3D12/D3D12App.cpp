#include "D3D12App.h"

#include "../Utils/Utils.h"
#include "../Managers/ImGuiManager.h"

#include "../D3DX12/d3dx12.h" // Helper structs for D3D12 (like CD3DX12_RESOURCE_BARRIER)

D3D12App::D3D12App(HWND hWnd, UINT width, UINT height)
{
	using namespace Microsoft::WRL;

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();

	Debug::Print(L"Debug Controller Load Complete!");

	ComPtr<ID3D12Debug1> debugController1;
	ThrowIfFailed(debugController.As(&debugController1))
	debugController1->SetEnableGPUBasedValidation(TRUE);
	
	Debug::Print(L"Debug Controller1 Load Complete!");
#endif // DEBUG

	// Create DXGI Factory
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
	Debug::Print(L"Factory2 Load Complete!");

	ThrowIfFailed(factory.As(&m_factory));
	Debug::Print(L"Factory4 Load Complete!");

	// Create D3D12 Device
	ThrowIfFailed(D3D12CreateDevice(
		nullptr, // default adapter
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&m_device)
	));
	
	Debug::Print(L"D3D12 Device Load Complete!");
	

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
	swapChainDesc.Width = width; // TODO: use window width
	swapChainDesc.Height = height; // TODO: use window height
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
	));
	Debug::Print(L"Swap Chain Load Complete!");
	

	ThrowIfFailed(swapChain.As(&m_swapChain))
	Debug::Print(L"Swap Chain3 Load Complete!");
	
	// This sample does not support fullscreen transitions.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create RTV Heaps
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 2; // double buffering
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		Debug::Print(L"Render Target View Heap Load Complete!");


		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT n = 0; n < 2; n++)
		{
			// 스왑 체인으로부터 n번째 버퍼 리소스를 가져옵니다.
			m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargetBuffers[n]));
			// 해당 버퍼에 대한 RTV를 생성하여 힙에 저장합니다.
			m_device->CreateRenderTargetView(m_renderTargetBuffers[n].Get(), nullptr, rtvHandle);
			// 다음 RTV를 저장할 위치로 핸들을 이동합니다.
			rtvHandle.ptr += m_rtvDescriptorSize;

			Debug::Print(L"Render Target View [" + std::to_wstring(n) + L"] Load Complete!");
		}
	}

	{
		// Create DSV Heap
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1; // 깊이 버퍼는 보통 1개만 사용합니다.
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		// 깊이/스텐실 버퍼를 위한 리소스를 생성합니다.
		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT; // 32비트 깊이 포맷
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		// 최적의 Clear 값을 설정합니다.
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
	}

	// Create Command Allocators
	for(UINT n = 0; n < FRAME_COUNT; n++)
	{
		ThrowIfFailed(m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocators[n])
		));
		
		Debug::Print(L"Command Allocator [" + std::to_wstring(n) + L"] Load Complete!");
		
	}

	// Create Command List
	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[m_frameIndex].Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	));
	Debug::Print(L"Command List Load Complete!");
	

	m_commandList->Close();

	// Fence & Event
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	Debug::Print(L"Fence Load Complete!");
	
	m_fenceValue = 0;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// Load Shaders

	auto dir = GetExeDirectory();
	auto vsBytecode = ReadShaderBytecode(dir + L"\\SimpleVS.cso");
	auto psBytecode = ReadShaderBytecode(dir + L"\\SimplePS.cso");

	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.pShaderBytecode = vsBytecode.data();
	vertexShaderBytecode.BytecodeLength = vsBytecode.size();

	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.pShaderBytecode = psBytecode.data();
	pixelShaderBytecode.BytecodeLength = psBytecode.size();
	
	Debug::Print(L"Shader Bytecode Load Complete!");

	// Create Root Signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error));

	ThrowIfFailed(m_device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)));
	
	Debug::Print(L"Root Signature Load Complete!");
	

	// Create Pipeline State Object(PSO)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Tangent),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Bitangent),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_renderTargetBuffers[0]->GetDesc().Format;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));
	Debug::Print(L"Pipeline State Object Load Complete!");
	

	SetViewportAndScissorRect(width, height);

	ModelManager::LoadModel("Dragon 2.5_fbx.fbx", m_vertices, m_indices);
	Debug::Print(L"Model Load Complete! Vertex Count: " + std::to_wstring(m_vertices.size()) + L", Index Count: " + std::to_wstring(m_indices.size()));

	
	UINT vbSize = static_cast<UINT>(m_vertices.size() * sizeof(Vertex));
	UINT ibSize = static_cast<UINT>(m_indices.size() * sizeof(uint32_t));

	// 정점 버퍼 뷰
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_vertexBuffer))
	);

	void* vbData;
	CD3DX12_RANGE readRange(0, 0);
	m_vertexBuffer->Map(0, &readRange, &vbData);
	memcpy(vbData, m_vertices.data(), vbSize);
	m_vertexBuffer->Unmap(0, nullptr);

	m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vbView.SizeInBytes = vbSize;
	m_vbView.StrideInBytes = sizeof(Vertex);

	// 인덱스 버퍼 뷰
	CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer))
	);

	void* ibData;
	m_indexBuffer->Map(0, &readRange, &ibData);
	memcpy(ibData, m_indices.data(), ibSize);
	m_indexBuffer->Unmap(0, nullptr);

	m_ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_ibView.SizeInBytes = ibSize;
	m_ibView.Format = DXGI_FORMAT_R32_UINT;


#ifdef _DEBUG
	m_imguiManager = std::make_unique<ImGuiManager>
		(m_device.Get(), m_commandQueue.Get(), hWnd, FRAME_COUNT);
	Debug::Print(L"ImGui Manager Load Complete!");
#endif // _DEBUG
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

	// 루트 시그니처와 PSO를 파이프라인에 설정합니다.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetPipelineState(m_pso.Get());

	// 뷰포트와 시저렉트 설정 (이전에 추가했어야 함)
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Rendering commands
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargetBuffers[m_frameIndex].Get();
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

	m_commandList->IASetVertexBuffers(0, 1, &m_vbView); // 정점 버퍼 설정
	m_commandList->IASetIndexBuffer(&m_ibView); // 인덱스 버퍼 설정
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 프리미티브 토폴로지 설정 (어떤 종류의 도형을 그릴지)

	// 3. 그리기 명령!
	// 정점 3개, 인스턴스 1개를 그리라는 명령입니다.
	// 이 명령이 실행되면 VSMain이 3번 (vertexID = 0, 1, 2) 호출됩니다.
	m_commandList->DrawIndexedInstanced(
		static_cast<UINT>(m_indices.size()),
		1,
		0,
		0,
		0
	);

	//새로운 ImGui 프레임 시작
#ifdef _DEBUG
	m_imguiManager->NewFrame();
	m_imguiManager->Render(m_commandList.Get());
#endif // _DEBUG

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
	if(width == m_viewport.Width && height == m_viewport.Height)
		return;

	WaitForGPU();

	// 기존의 렌더 타겟을 해제합니다.
	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		m_renderTargetBuffers[n].Reset();
	}

	// 스왑 체인의 버퍼 크기를 변경합니다.
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		FRAME_COUNT,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0
	));

	// m_frameIndex를 최신 상태로 유지합니다.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// 각 버퍼에 대한 RTV를 다시 생성합니다.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargetBuffers[n]));
		m_device->CreateRenderTargetView(m_renderTargetBuffers[n].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}

	SetViewportAndScissorRect(width, height);
}

void D3D12App::SetViewportAndScissorRect(UINT width, UINT height)
{
	// 뷰포트(Viewport) 설정
	// 뷰포트는 렌더링 결과물이 그려질 렌더 타겟의 영역을 정의합니다.
	m_viewport.TopLeftX = 0.0f;             // 렌더링 영역의 왼쪽 위 X 좌표
	m_viewport.TopLeftY = 0.0f;             // 렌더링 영역의 왼쪽 위 Y 좌표
	m_viewport.Width = static_cast<float>(width);   // 렌더링 영역의 너비
	m_viewport.Height = static_cast<float>(height); // 렌더링 영역의 높이
	m_viewport.MinDepth = 0.0f;             // 깊이 버퍼의 최소 깊이 값 (항상 0.0f)
	m_viewport.MaxDepth = 1.0f;             // 깊이 버퍼의 최대 깊이 값 (항상 1.0f)

	Debug::Print(L"Viewport Set: " + std::to_wstring(width) + L"x" + std::to_wstring(height));

	// 시저 렉트(Scissor Rectangle) 설정
	// 시저 렉트는 픽셀 셰이더의 영향을 받는 영역을 정의합니다. 이 사각형 바깥의 픽셀들은
	// 렌더링되지 않습니다 (가위로 잘라내는 효과).
	m_scissorRect.left = 0;                 // 잘라낼 사각형의 왼쪽 좌표
	m_scissorRect.top = 0;                  // 잘라낼 사각형의 위쪽 좌표
	m_scissorRect.right = width;          // 잘라낼 사각형의 오른쪽 좌표
	m_scissorRect.bottom = height;        // 잘라낼 사각형의 아래쪽 좌표

	Debug::Print(L"Scissor Rect Set: " + std::to_wstring(width) + L"x" + std::to_wstring(height));
}