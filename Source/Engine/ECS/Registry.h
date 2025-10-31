// Registry.h (Facade)
#pragma once

#include "EntityManager.h"
#include "ComponentManager.h"

class Registry
{
public:
    // 1. 단순한 API 제공 (생성)
    Entity CreateEntity()
    {
        // 복잡한 로직은 내부 EntityManager가 알아서 함
        return m_entityManager.CreateEntity();
    }

    // 2. 단순한 API 제공 (파괴)
    void DestroyEntity(Entity entity)
    {
        // [퍼사드의 핵심 가치]
        // 사용자는 'DestroyEntity' 하나만 호출했지만,
        // 이 Facade는 내부적으로 '반드시' 호출되어야 하는
        // 두 개의 서브시스템을 '올바른 순서로' 호출해 줍니다.
        // 1. 모든 컴포넌트 데이터를 먼저 제거하고,
        m_componentManager.EntityDestroyed(entity);
        // 2. 그 다음에 ID를 재활용 큐에 반환합니다.
        m_entityManager.DestroyEntity(entity);
    }

    // 3. 단순한 API 제공 (컴포넌트 추가)
    template<typename T, typename... Args>
    T& AddComponent(Entity entity, Args&&... args)
    {
        // 사용자는 ComponentManager의 존재를 알 필요가 없음
        return m_componentManager.AddComponent<T>(entity, std::forward<Args>(args)...);
    }

    // ... GetComponent, RemoveComponent 등 모든 API를 위임(delegate) ...

private:
    // 이 복잡한 시스템들은 'Registry'라는 벽 뒤에
    // private으로 완벽하게 숨겨집니다.
    EntityManager    m_entityManager;
    ComponentManager m_componentManager;
};