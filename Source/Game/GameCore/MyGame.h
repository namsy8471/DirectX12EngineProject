#pragma once

// Engine 프로젝트의 D3D12App.h를 include 합니다.
#include "Core/D3D12App.h" 

// 게임 로직은 '엔진'이 아닌 '게임'에만 include 됩니다.
#include "Managers/ModelManager.h"
#include "ECS/Registry.h" // (ECS 사용 예시)
#include "imgui.h" // ImGui::ImTextureID

/*
 * MyGame은 D3D12App 프레임워크를 '구현'하는 구체적인(Concrete) 클래스입니다.
 * 이 클래스가 '우리 게임'에 필요한 리소스(PSO, 메쉬)를 소유합니다.
*/

class MyGame : public D3D12App
{
public:
	MyGame(HINSTANCE hInstance);
	virtual ~MyGame();

	virtual bool Init(HWND hWnd, UINT width, UINT height) override;
	virtual void Update(float dt) override;
	virtual void Shutdown() override;

protected:
	void DrawGame();
	virtual void OnResize(UINT width, UINT height) override;

private:

	// ECS 시스템 (예시)
	Registry m_registry; 
	// TransformSystem m_transformSystem;
	// RenderSystem m_renderSystem;

	// 리소스 관리자
	ModelManager m_modelManager;

	// Render To Texture용 리소스와 뷰 힙
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rttRtvHeap;	// 렌더 타겟 뷰 힙
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rttDsvHeap;  // 깊이 스텐실 뷰 힙

	// Scene View RTT 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> m_sceneTexture; // Scene 렌더 타겟 텍스처
	Microsoft::WRL::ComPtr<ID3D12Resource> m_sceneDepthBuffer; // Scene 깊이 버퍼 텍스처
	D3D12_CPU_DESCRIPTOR_HANDLE m_sceneRtvHandle; // RTT RTV 힙의 0번 슬롯
	D3D12_CPU_DESCRIPTOR_HANDLE m_sceneDsvHandle; // RTT DSV 힙의 0번 슬롯
	ImTextureID m_sceneImGuiHandle; // ImGui용 텍스처 핸들
	//Camera m_sceneCamera;
	ImVec2 m_sceneViewportSize = { 1280, 720 }; // ImGui에 표시할 때의 크기

	// Game View RTT 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> m_gameTexture; // Scene 렌더 타겟 텍스처
	Microsoft::WRL::ComPtr<ID3D12Resource> m_gameDepthBuffer; // Scene 깊이 버퍼 텍스처
	D3D12_CPU_DESCRIPTOR_HANDLE m_gameRtvHandle; // RTT RTV 힙의 0번 슬롯
	D3D12_CPU_DESCRIPTOR_HANDLE m_gameDsvHandle; // RTT DSV 힙의 0번 슬롯
	ImTextureID m_gameImGuiHandle; // ImGui용 텍스처 핸들
	//Camera m_gameCamera;
	ImVec2 m_gameViewportSize = { 1280, 720 }; // ImGui에 표시할 때의 크기


	// 게임 리소스 (PSO, RootSig, Mesh 등)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;

	std::vector<Vertex> m_vertices;
	std::vector<UINT> m_indices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vbView;
	D3D12_INDEX_BUFFER_VIEW m_ibView;
};