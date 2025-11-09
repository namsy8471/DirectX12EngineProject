#include "ImGuiManager.h"

#include "../Utils/Utils.h"
#include "Components/Camera.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <d3d12.h>

ImGuiManager* ImGuiManager::s_instance = nullptr; // 싱글톤 초기화

ImGuiManager::ImGuiManager(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, HWND hWnd, const int FRAME_COUNT)
{
	ImGuiManager::s_instance = this;
	m_device = device;

	// Create ImGui Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
	imguiHeapDesc.NumDescriptors = m_totalHeapSize;
	imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&m_imguiDescHeap));

	// Get SRV Descriptor Size
	m_srvHandleIncrementSize = 
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 폰트 디스크립터 핸들
	D3D12_CPU_DESCRIPTOR_HANDLE fontCpuHandle = 
		m_imguiDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE fontGpuHandle = 
		m_imguiDescHeap->GetGPUDescriptorHandleForHeapStart();

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

	m_imGuiDescHeapAlloc.Create(device, m_imguiDescHeap.Get());

	// 기존의 ImGui 초기화 방식 (더 이상 사용하지 않음)
	/*ImGui_ImplDX12_Init(device, FRAME_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM, m_imguiDescHeap.Get(),
		m_imguiDescHeap->GetCPUDescriptorHandleForHeapStart(), m_imguiDescHeap->GetGPUDescriptorHandleForHeapStart());*/

	// 아래는 ImGui 1.89.5 버전부터 지원하는 새로운 초기화 방식 (콜백함수 등록)
	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = device;
	init_info.CommandQueue = cmdQueue;
	init_info.NumFramesInFlight = FRAME_COUNT;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	init_info.SrvDescriptorHeap = m_imguiDescHeap.Get();
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
		{
			ImGuiManager::s_instance->m_imGuiDescHeapAlloc.Alloc(cpu, gpu);
		};
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
		{
			ImGuiManager::s_instance->m_imGuiDescHeapAlloc.Free(cpu, gpu);
		};

	ImGui_ImplDX12_Init(&init_info);

	// 초기 창 크기 설정
	RECT rect;
	GetClientRect(hWnd, &rect);
	SendMessage(hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));
}

ImGuiManager::~ImGuiManager()
{
	ImGui_ImplDX12_Shutdown();
	m_imGuiDescHeapAlloc.Destroy();
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

ImTextureID ImGuiManager::RegisterTexture(ID3D12Resource* pTextureResource)
{
	// Check if heap is full
	if (m_currentHeapIndex >= m_totalHeapSize)
	{
		Debug::Print("ImGui Descriptor Heap is Full!");
		throw std::runtime_error("ImGui Descriptor Heap is Full!");
	}

	// Create SRV for the texture
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_imguiDescHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += m_srvHandleIncrementSize * m_currentHeapIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_imguiDescHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += m_srvHandleIncrementSize * m_currentHeapIndex;

	// Describe and create the SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = pTextureResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_device->CreateShaderResourceView(pTextureResource, &srvDesc, cpuHandle);

	m_currentHeapIndex++;
	return static_cast<ImTextureID>(gpuHandle.ptr);
}
