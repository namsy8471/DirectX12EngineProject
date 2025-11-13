#pragma once

class GameObject;

class IComponent
{
public:
	IComponent(GameObject* owner) : m_owner(owner) {}
	virtual ~IComponent() = default;

	// Interface methods
	virtual void Init() {};
	virtual void Update([[maybe_unused]]float deltaTime) {};
	GameObject* GetOwner() const { return m_owner; };

private:
	GameObject* m_owner;
};

