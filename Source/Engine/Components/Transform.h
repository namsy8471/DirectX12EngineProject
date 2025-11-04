#pragma once

#include <DirectXMath.h>
#include "IComponent.h"

class GameObject;	// Forward declaration
using namespace DirectX;

class Transform : public IComponent
{
public:
	Transform(GameObject* owner) : IComponent(owner) {}
	virtual ~Transform() = default;

	// 위치 설정 및 가져오기
	XMFLOAT3 GetPosition() const { return m_Position; }
	void SetPosition(const XMFLOAT3& position) { m_Position = position; }
	void SetPosition(float x, float y, float z) { m_Position = XMFLOAT3(x, y, z); }
	void Translate(const XMFLOAT3& delta);
	
	// 회전 설정 및 가져오기 (쿼터니언)
	XMFLOAT4 GetRotation() const { return m_Rotation; }
	void SetRotation(const XMFLOAT4& quaterinion) { m_Rotation = quaterinion; }
	void SetRotation(float x, float y, float z, float w) { m_Rotation = XMFLOAT4(x, y, z, w); }
	void Rotate(const XMFLOAT4& quaterinion);

	// 스케일 설정 및 가져오기
	XMFLOAT3 GetScale() const { return m_Scale; }
	void SetScale(const XMFLOAT3& scale) { m_Scale = scale; }

	XMFLOAT3 GetForward() const;
	XMFLOAT3 GetUp() const;
	XMFLOAT3 GetRight() const;

	XMMATRIX GetWorldMatrix() const;

private:
	XMFLOAT3  m_Position = { 0.0f, 0.0f, 0.0f };	   // Translation
	XMFLOAT4  m_Rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; // Quaternion
	XMFLOAT3  m_Scale = { 1.0f, 1.0f, 1.0f };		   // Scale
};