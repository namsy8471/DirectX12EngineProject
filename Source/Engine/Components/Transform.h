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
	XMFLOAT3 GetPosition() const { return m_position; }
	void SetPosition(const XMFLOAT3& position) { m_position = position; }
	void SetPosition(float x, float y, float z) { m_position = XMFLOAT3(x, y, z); }
	void Translate(const XMFLOAT3& delta);
	
	// 회전 설정 및 가져오기 (쿼터니언)
	XMFLOAT4 GetRotation() const { return m_rotation; }
	void SetRotation(const XMFLOAT4& quaterinion) { m_rotation = quaterinion; }
	void SetRotation(float x, float y, float z, float w) { m_rotation = XMFLOAT4(x, y, z, w); }
	void Rotate(const XMFLOAT4& quaterinion);

	// 스케일 설정 및 가져오기
	XMFLOAT3 GetScale() const { return m_scale; }
	void SetScale(const XMFLOAT3& scale) { m_scale = scale; }

	XMFLOAT3 GetForward() const;
	XMFLOAT3 GetUp() const;
	XMFLOAT3 GetRight() const;

	XMMATRIX GetWorldMatrix() const;

private:
	XMFLOAT3  m_position = { 0.0f, 0.0f, 0.0f };	   // Translation
	XMFLOAT4  m_rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; // Quaternion
	XMFLOAT3  m_scale = { 1.0f, 1.0f, 1.0f };		   // Scale
};