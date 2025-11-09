#include "Transform.h"

// 위치를 delta만큼 이동시키는 함수
void Transform::Translate(const XMFLOAT3& delta)
{
	XMVECTOR currentPos = XMLoadFloat3(&m_position);
	XMVECTOR deltaVec = XMLoadFloat3(&delta);
	XMVECTOR newPos = XMVectorAdd(currentPos, deltaVec);
	
	XMStoreFloat3(&m_position, newPos);
}

// 회전을 쿼터니언으로 적용하는 함수
void Transform::Rotate(const XMFLOAT4& quaterinion)
{
	XMVECTOR currentRot = XMLoadFloat4(&m_rotation);
	XMVECTOR deltaRot = XMLoadFloat4(&quaterinion);
	XMVECTOR newRot = XMQuaternionMultiply(currentRot, deltaRot);
	
	XMStoreFloat4(&m_rotation, newRot);
}

// 앞 방향 벡터 반환
XMFLOAT3 Transform::GetForward() const
{
	static const XMVECTOR V_FORWARD = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR rotationQuat = XMLoadFloat4(&m_rotation);

	XMVECTOR ret = XMVector3Rotate(V_FORWARD, rotationQuat);

	XMFLOAT3 forward;
	XMStoreFloat3(&forward, ret);

    return forward;
}

// 위 방향 벡터 반환
XMFLOAT3 Transform::GetUp() const
{
	static const XMVECTOR V_UP = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR rotationQuat = XMLoadFloat4(&m_rotation);

	XMVECTOR ret = XMVector3Rotate(V_UP, rotationQuat);
	
	XMFLOAT3 up;
	XMStoreFloat3(&up, ret);
    
	return up;
}

// 오른쪽 방향 벡터 반환
XMFLOAT3 Transform::GetRight() const
{
	static const XMVECTOR V_RIGHT = { 1.0f, 0.0f, 0.0f, 0.0f };
	XMVECTOR rotationQuat = XMLoadFloat4(&m_rotation);

	XMVECTOR ret = XMVector3Rotate(V_RIGHT, rotationQuat);

	XMFLOAT3 right;
	XMStoreFloat3(&right, ret);

	return right;
}

// 월드 변환 행렬 반환
XMMATRIX Transform::GetWorldMatrix() const
{
	XMVECTOR scaleVec = XMLoadFloat3(&m_scale);
	XMVECTOR rotationQuat = XMLoadFloat4(&m_rotation);
	XMVECTOR positionVec = XMLoadFloat3(&m_position);

	XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scaleVec);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotationQuat);
	XMMATRIX translationMatrix = XMMatrixTranslationFromVector(positionVec);

	return scaleMatrix * rotationMatrix * translationMatrix;
}
