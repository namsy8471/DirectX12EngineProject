#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"

// 카메라 컴포넌트 생성자
Camera::Camera(GameObject* owner) : IComponent(owner),
	m_fov(60.0f),
	m_aspectRatio(16.0f / 9.0f),
	m_nearPlane(0.1f),
	m_farPlane(1000.0f)
{
	
}

void Camera::Init()
{
	// 초기 뷰 및 투영 행렬 설정
	DirectX::XMStoreFloat4x4(&m_viewMatrix, GetViewMatrix());
	SetProjectionMatrix(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
}

// 뷰 행렬 계산
XMMATRIX Camera::GetViewMatrix() const
{
	Transform* transform = GetOwner()->GetComponent<Transform>();

	XMFLOAT3 pos = transform->GetPosition();
	XMFLOAT3 forward = transform->GetForward();
	XMFLOAT3 up = transform->GetUp();

	XMVECTOR posVec = XMLoadFloat3(&pos);
	XMVECTOR forwardVec = XMLoadFloat3(&forward);
	XMVECTOR upVec = XMLoadFloat3(&up);

	XMVECTOR target = XMVectorAdd(posVec, forwardVec);
	return XMMatrixLookAtLH(posVec, target, upVec);
}

// 투영 행렬 가져오기
XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMLoadFloat4x4(&m_projectionMatrix);
}

// 투영 행렬 설정
void Camera::SetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	m_fov = fov;
	m_aspectRatio = aspectRatio;
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
	XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
	XMStoreFloat4x4(&m_projectionMatrix, projMatrix);	
}
