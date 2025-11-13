#pragma once

// Engine 프로젝트의 D3D12App.h를 include 합니다.
#include "Core/D3D12App.h" 

// 게임 로직은 '엔진'이 아닌 '게임'에만 include 됩니다.
#include "Managers/ModelManager.h"
//#include "ECS/Registry.h" // (ECS 사용 예시)


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
	virtual void Render3DScene(const Camera& camera) override;
	virtual void OnResize(UINT width, UINT height) override;

private:

	// ECS 시스템 (예시)
	//Registry m_registry; 
	// TransformSystem m_transformSystem;
	// RenderSystem m_renderSystem;

	// 리소스 관리자
	ModelManager m_modelManager;

	// 활성 씬
	std::unique_ptr<Scene> m_activeScene;

	// 게임 리소스 (PSO, RootSig, Mesh 등)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;

	std::vector<Vertex> m_vertices;
	std::vector<UINT> m_indices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vbView;
	D3D12_INDEX_BUFFER_VIEW m_ibView;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer; // 상수 버퍼
	UINT8* m_pCbvDataBegin = nullptr; // 상수 버퍼 매핑 포인터
};