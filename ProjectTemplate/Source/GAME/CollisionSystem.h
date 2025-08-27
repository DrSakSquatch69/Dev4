#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "../CCL.h"
#include "GameComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME {
    // Check if two OBBs are colliding
    bool CheckCollision(const GW::MATH::GOBBF& obb1, const GW::MATH::GMATRIXF& transform1,
                        const GW::MATH::GOBBF& obb2, const GW::MATH::GMATRIXF& transform2);

    // Update the collider's world position based on the entity's transform
    void UpdateColliderTransform(entt::registry& registry, entt::entity entity);

    // Initialize the collision system
    void InitializeCollisionSystem(entt::registry& registry);

    GW::MATH::GOBBF TransformOBBToWorldSpace(const GW::MATH::GOBBF& localOBB, const GW::MATH::GMATRIXF& transform);

    bool CheckOBBCollision(const GW::MATH::GOBBF& obb1, const GW::MATH::GOBBF& obb2);

}

#endif // COLLISION_SYSTEM_H