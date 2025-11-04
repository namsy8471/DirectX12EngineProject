#include "MyGame.h"

#include "Utils/Utils.h" // GetExeDirectory, ReadShaderBytecode
#include "D3DX12/d3dx12.h" // CD3DX12

#include "Components/Scene.h"
#include "Components/GameObject.h"
#include "Components/Camera.h"

// 생성자에서 부모(D3D12App) 생성자를 호출
MyGame::MyGame(HINSTANCE hInstance)
	: D3D12App(hInstance)
{
	Debug::Print(L"MyGame Created!");
}

MyGame::~MyGame()
{
	Shutdown();
}

// '게임'에 특화된 리소스를 초기화합니다.
// (D3D12App::InitBase()가 성공적으로 끝난 '이후'에 호출됩니다)
bool MyGame::Init(HWND hWnd, UINT width, UINT height)
{
	using namespace Microsoft::WRL;

	InitBase(hWnd, width, height); // 부모 클래스의 InitBase 호출

	m_EditorCameraObject = std::make_unique<GameObject>("Editor Camera");
	m_EditorCameraObject->GetTransform()->SetPosition(0.0f, 5.0f, -10.0f);
	m_EditorCamera = m_EditorCameraObject->AddComponent<Camera>();

	m_ActiveScene = std::make_unique<Scene>("Main Scene");
	Debug::Print(L"Scene and Editor Camera Created!");

	GameObject* gameCamObj = m_ActiveScene->CreateGameObject("Main Camera");
	gameCamObj->GetTransform()->SetPosition(0.0f, 1.0f, -5.0f);
	Camera* gameCam = gameCamObj->AddComponent<Camera>();

	m_ActiveScene->SetMainCamera(gameCam);
	Debug::Print(L"Main Camera Created in Scene!");

	m_ActiveScene->CreateGameObject("Player");
	m_ActiveScene->CreateGameObject("Ground");


	// Load Shaders
	auto dir = GetExeDirectoryWstring();
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
	D3D12_DESCRIPTOR_RANGE descriptorRange = {};
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorRange.NumDescriptors = 1;
	descriptorRange.BaseShaderRegister = 0; // b0

	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.DescriptorTable.NumDescriptorRanges = 1;
	rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRange;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	Debug::Print(L"Root Signature Load Complete!");

	// Create Pipeline State Object(PSO)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;

	// ... (RasterizerState, BlendState)
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Input Layout
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
	// m_renderTargetBuffers는 부모 클래스(D3D12App)에
	// 'protected'로 선언되어 있으므로 자식 클래스에서 접근 가능합니다.
	psoDesc.RTVFormats[0] = m_renderTargetBuffers[0]->GetDesc().Format;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	
	// DSV 포맷도 설정해야 합니다.
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	
	// 깊이 테스트를 켜야 합니다.
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;


	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_Pso)));
	Debug::Print(L"Pipeline State Object Load Complete!");

	// Load Model Data
	m_ModelManager.LoadModel("Dragon 2.5_fbx.fbx", m_Vertices, m_Indices);
	Debug::Print(L"Model Load Complete! Vertex Count: " + std::to_wstring(m_Vertices.size()) + L", Index Count: " + std::to_wstring(m_Indices.size()));

	// Create Vertex Buffer
	UINT vbSize = static_cast<UINT>(m_Vertices.size() * sizeof(Vertex));
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexBuffer))
	);
	void* vbData;
	m_VertexBuffer->Map(0, nullptr, &vbData);
	memcpy(vbData, m_Vertices.data(), vbSize);
	m_VertexBuffer->Unmap(0, nullptr);

	m_VbView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VbView.SizeInBytes = vbSize;
	m_VbView.StrideInBytes = sizeof(Vertex);

	// Create Index Buffer
	UINT ibSize = static_cast<UINT>(m_Indices.size() * sizeof(UINT));
	CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexBuffer))
	);
	void* ibData;
	m_IndexBuffer->Map(0, nullptr, &ibData);
	memcpy(ibData, m_Indices.data(), ibSize);
	m_IndexBuffer->Unmap(0, nullptr);

	m_IbView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IbView.SizeInBytes = ibSize;
	m_IbView.Format = DXGI_FORMAT_R32_UINT;

	return true; // 초기화 성공
}

// '게임 로직' 업데이트. D3D12App::Run()의 메인 루프에서 매 프레임 호출됩니다.
void MyGame::Update(float deltaTime)
{
	m_ActiveScene->Update(deltaTime);
}

// '그리기' 명령. D3D12App::Render() 내부에서 호출됩니다.
// 엔진이 RTV/DSV 설정, 뷰포트/시저 설정을 '끝낸 후' 호출됩니다.
void MyGame::DrawGame()
{
	// [게임] PSO, RootSig 설정
	m_commandList->SetGraphicsRootSignature(m_RootSignature.Get());
	m_commandList->SetPipelineState(m_Pso.Get());

	// [게임] 리소스 바인딩 (ECS RenderSystem이 이 작업을 할 것입니다)
	m_commandList->IASetVertexBuffers(0, 1, &m_VbView); // 정점 버퍼 설정
	m_commandList->IASetIndexBuffer(&m_IbView); // 인덱스 버퍼 설정
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// [게임] 그리기
	m_commandList->DrawIndexedInstanced(
		static_cast<UINT>(m_Indices.size()),
		1, 0, 0, 0
	);
}

// 엔진이 리사이즈를 끝낸 후 호출됩니다.
// 게임이 소유한 리소스(예: G-Buffer)를 리사이즈할 때 씁니다.
void MyGame::OnResize(UINT width, UINT height)
{
	Debug::Print(L"MyGame OnResize: " + std::to_wstring(width) + L"x" + std::to_wstring(height));
	// (현재는 게임이 소유한 리사이즈 대상 리소스가 없으므로 비워둡니다.)
}

void MyGame::Shutdown()
{
}
