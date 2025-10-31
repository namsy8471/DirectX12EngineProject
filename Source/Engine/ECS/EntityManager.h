// Entity Manager.h
#pragma once

#include "ComponentArray.h" // Entity 타입을 가져오기 위해
#include <queue>              // 재활용할 ID를 저장하기 위한 큐
#include <set>                // 현재 활성화된 Entity를 빠르게 추적

/*
 * [EntityManager]
 * (수석님의 조언)
 * Entity ID의 생성(Create)과 파괴(Destroy), 재활용(Recycle)을
 * 전문적으로 담당합니다.
 */
class EntityManager
{
public:
    EntityManager()
    {
        // 0번 Entity는 보통 'Null' 또는 'Scene Root' 용으로 남겨두기도 하지만,
        // 여기서는 간단하게 0번부터 999번까지 미리 큐에 넣어둡니다.
        for (Entity e = 0; e < MAX_ENTITIES; ++e)
        {
            m_availableEntities.push(e);
        }
    }

    Entity CreateEntity()
    {
        assert(!m_availableEntities.empty() && "Entity limit reached!");

        // 1. 재활용 큐에서 사용 가능한 ID를 하나 꺼냅니다. (O(1))
        Entity id = m_availableEntities.front();
        m_availableEntities.pop();

        // 2. 현재 활성화된 Entity 집합에 추가합니다.
        m_activeEntities.insert(id);

        return id;
    }

    void DestroyEntity(Entity entity)
    {
        assert(m_activeEntities.count(entity) && "Destroying non-existent entity");

        // 1. 활성화 집합에서 제거합니다.
        m_activeEntities.erase(entity);

        // 2. 이 ID를 재활용 큐에 다시 넣습니다. (O(1))
        m_availableEntities.push(entity);
    }

    const std::set<Entity>& GetActiveEntities() const
    {
        return m_activeEntities;
    }

private:
    // (이론상) 32비트 ID의 최대값은 42억 개지만,
    // (실무상) '희소 배열(Sparse Array)'의 최대 크기를 제한하기 위해
    // 동시 활성화 가능한 Entity 개수 한도를 정하는 것이 좋습니다.
    static const Entity MAX_ENTITIES = 50000;

    std::queue<Entity> m_availableEntities; // 재활용 가능한 ID 저장소
    std::set<Entity> m_activeEntities;      // 현재 사용 중인 ID 저장소
};

