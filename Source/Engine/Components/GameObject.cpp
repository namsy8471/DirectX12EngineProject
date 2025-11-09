#include "GameObject.h"

GameObject::GameObject(std::string name) : m_name(name), m_transform(this)
{

}

GameObject::~GameObject()
{

}

void GameObject::Update(float deltaTime)
{
	for (auto& component : m_components)
	{
		component->Update(deltaTime);
	}
}