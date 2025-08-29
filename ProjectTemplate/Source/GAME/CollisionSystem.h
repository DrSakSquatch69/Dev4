#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "../CCL.h"
#include "GameComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME {
    bool CheckCollision(const GW::MATH::GOBBF& obb1, const GW::MATH::GMATRIXF& transform1,
                        const GW::MATH::GOBBF& obb2, const GW::MATH::GMATRIXF& transform2);

    void UpdateColliderTransform(entt::registry& registry, entt::entity entity);

    void HandleBulletEnemyCollision(entt::registry& registry, entt::entity bulletEntity, entt::entity enemyEntity);

    void HandlePlayerEnemyCollision(entt::registry& registry, entt::entity playerEntity, entt::entity enemyEntity);

    void HandleEntityObstacleCollision(entt::registry& registry, entt::entity entityEntity, entt::entity obstacleEntity);

    GW::MATH::GOBBF TransformOBBToWorldSpace(const GW::MATH::GOBBF& localOBB, const GW::MATH::GMATRIXF& transform);

    bool CheckOBBCollision(const GW::MATH::GOBBF& obb1, const GW::MATH::GOBBF& obb2);

    void collision_system_update(entt::registry& registry);

    void InitializeCollisionSystem(entt::registry& registry);

    void UpdateCollisionSystem(entt::registry& registry);

}

#endif // COLLISION_SYSTEM_H