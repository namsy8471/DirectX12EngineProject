#include "GameObject.h"

GameObject::GameObject(std::string name) : m_Name(name), m_Transform(this)
{

}

GameObject::~GameObject()
{

}

void GameObject::Update(float deltaTime)
{
	for (auto& component : m_Components)
	{
		component->Update(deltaTime);
	}
}