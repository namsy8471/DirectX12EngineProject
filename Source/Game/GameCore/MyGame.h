#pragma once

// Engine 프로젝트의 D3D12App.h를 include 합니다.
#include "Core/D3D12App.h" 

// 게임 로직은 '엔진'이 아닌 '게임'에만 include 됩니다.
#include "Managers/ModelManager.h"
//#include "ECS/Registry.h" // (ECS 사용 예시)
#include "imgui.h" // ImGui::ImTextureID

#include <memory>
#include <vector>

class Scene;
class GameObject;
class Camera;

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
	//Registry m_registry; 
	// TransformSystem m_transformSystem;
	// RenderSystem m_renderSystem;

	// 리소스 관리자
	ModelManager m_ModelManager;

	// 활성 씬
	std::unique_ptr<Scene> m_ActiveScene;

	// Editor Camera
	std::unique_ptr<GameObject> m_EditorCameraObject;
	Camera* m_EditorCamera = nullptr;

	// Render To Texture용 리소스와 뷰 힙
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RttRtvHeap;	// 렌더 타겟 뷰 힙
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RttDsvHeap;  // 깊이 스텐실 뷰 힙

	// Scene View RTT 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> m_SceneTexture; // Scene 렌더 타겟 텍스처
	Microsoft::WRL::ComPtr<ID3D12Resource> m_SceneDepthBuffer; // Scene 깊이 버퍼 텍스처
	D3D12_CPU_DESCRIPTOR_HANDLE m_SceneRtvHandle; // RTT RTV 힙의 0번 슬롯
	D3D12_CPU_DESCRIPTOR_HANDLE m_SceneDsvHandle; // RTT DSV 힙의 0번 슬롯
	ImTextureID m_SceneViewImGuiHandle = 0; // ImGui용 텍스처 핸들
	ImVec2 m_SceneViewportSize = { 1280, 720 }; // ImGui에 표시할 때의 크기

	// Game View RTT 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> m_GameTexture; // Scene 렌더 타겟 텍스처
	Microsoft::WRL::ComPtr<ID3D12Resource> m_GameDepthBuffer; // Scene 깊이 버퍼 텍스처
	D3D12_CPU_DESCRIPTOR_HANDLE m_GameRtvHandle; // RTT RTV 힙의 0번 슬롯
	D3D12_CPU_DESCRIPTOR_HANDLE m_GameDsvHandle; // RTT DSV 힙의 0번 슬롯
	ImTextureID m_GameViewImGuiHandle = 0; // ImGui용 텍스처 핸들
	//Camera m_gameCamera;
	ImVec2 m_GameViewportSize = { 1280, 720 }; // ImGui에 표시할 때의 크기


	// 게임 리소스 (PSO, RootSig, Mesh 등)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;

	std::vector<Vertex> m_Vertices;
	std::vector<UINT> m_Indices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_VbView;
	D3D12_INDEX_BUFFER_VIEW m_IbView;
};