#include "ImGuiManager.h"

#include "../Utils/Utils.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <d3d12.h>

ImGuiManager* ImGuiManager::s_instance = nullptr; // 싱글톤 초기화

ImGuiManager::ImGuiManager(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, HWND hWnd, const int FRAME_COUNT)
{
	UNREFERENCED_PARAMETER(cmdQueue);

	ImGuiManager::s_instance = this;

	// Create ImGui Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
	imguiHeapDesc.NumDescriptors = 64;
	imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&m_imguiDescHeap));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	
	// 스타일 설정
	ImGui::StyleColorsDark();

	// 백엔드 초기화
	ImGui_ImplWin32_Init(hWnd);

	m_ImGuiDescHeapAlloc.Create(device, m_imguiDescHeap.Get());

	// 기존의 ImGui 초기화 방식 (더 이상 사용하지 않음)
	/*ImGui_ImplDX12_Init(device, FRAME_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM, m_imguiDescHeap.Get(),
		m_imguiDescHeap->GetCPUDescriptorHandleForHeapStart(), m_imguiDescHeap->GetGPUDescriptorHandleForHeapStart());*/

	// 아래는 ImGui 1.89.5 버전부터 지원하는 새로운 초기화 방식 (콜백함수 등록)
	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = device;
	init_info.CommandQueue = cmdQueue;
	init_info.NumFramesInFlight = FRAME_COUNT;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.SrvDescriptorHeap = m_imguiDescHeap.Get();
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
		{
			ImGuiManager::s_instance->m_ImGuiDescHeapAlloc.Alloc(cpu, gpu);
		};
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
		{
			ImGuiManager::s_instance->m_ImGuiDescHeapAlloc.Free(cpu, gpu);
		};

	ImGui_ImplDX12_Init(&init_info);
}

ImGuiManager::~ImGuiManager()
{
	ImGui_ImplDX12_Shutdown();
	m_ImGuiDescHeapAlloc.Destroy();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiManager::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

}

void ImGuiManager::Render(ID3D12GraphicsCommandList* cmdList)
{
	ImGuiID d = ImGui::GetID("MyWindow");

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
	ImGui::PopStyleVar(3);

	ImGui::DockSpace(d, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();


	// Render your ImGui elements here
	// Sample ImGui Window
	ImGui::Begin("Hello, ImGui!");
	ImGui::Text("DirectX 12 + ImGui is Connected Successfully!");
	ImGui::End();

	ImGui::Render();

	ID3D12DescriptorHeap* heaps[] = { m_imguiDescHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(NULL, static_cast<void*>(cmdList));
	}
}
