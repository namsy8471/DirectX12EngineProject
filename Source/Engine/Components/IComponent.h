#pragma once

class GameObject;

class IComponent
{
public:
	IComponent(GameObject* owner) : m_Owner(owner) {}
	virtual ~IComponent() = default;

	// Interface methods
	virtual void Init() {};
	virtual void Update(float deltaTime) {}; 
	GameObject* GetOwner() const { return m_Owner; };

private:
	GameObject* m_Owner;
};

