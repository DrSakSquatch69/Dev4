#include "CollisionSystem.h"
#include "ModelManager.h"
#include <d3d12.h>

namespace GAME {

    // Check if two OBBs are colliding
    bool CheckCollision(const GW::MATH::GOBBF& obb1, const GW::MATH::GMATRIXF& transform1,
        const GW::MATH::GOBBF& obb2, const GW::MATH::GMATRIXF& transform2) {
        // Transform the OBBs to world space
        GW::MATH::GOBBF worldObb1 = obb1;
        GW::MATH::GOBBF worldObb2 = obb2;

        // Update centers based on transforms
        GW::MATH::GVECTORF worldCenter1 = { 0.0f, 0.0f, 0.0f, 1.0f };
        GW::MATH::GVECTORF worldCenter2 = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Use VectorXMatrixF instead of MultiplyVectorF
        GW::MATH::GMatrix::VectorXMatrixF(transform1, obb1.center, worldCenter1);
        GW::MATH::GMatrix::VectorXMatrixF(transform2, obb2.center, worldCenter2);

        worldObb1.center = worldCenter1;
        worldObb2.center = worldCenter2;

        // Simple AABB check for now (approximation)
        // Calculate half extents for each OBB
        float halfExtent1X = worldObb1.extent.x;
        float halfExtent1Y = worldObb1.extent.y;
        float halfExtent1Z = worldObb1.extent.z;

        float halfExtent2X = worldObb2.extent.x;
        float halfExtent2Y = worldObb2.extent.y;
        float halfExtent2Z = worldObb2.extent.z;

        // Check for overlap in all three axes
        bool overlapX = std::abs(worldCenter1.x - worldCenter2.x) < (halfExtent1X + halfExtent2X);
        bool overlapY = std::abs(worldCenter1.y - worldCenter2.y) < (halfExtent1Y + halfExtent2Y);
        bool overlapZ = std::abs(worldCenter1.z - worldCenter2.z) < (halfExtent1Z + halfExtent2Z);

        // Collision occurs if there's overlap in all axes
        return overlapX && overlapY && overlapZ;
    }

    // Update the collider's world position based on the entity's transform
    void UpdateColliderTransform(entt::registry& registry, entt::entity entity) {
        if (registry.all_of<Transform, MeshCollection>(entity)) {
            auto& transform = registry.get<Transform>(entity);
            auto& meshCollection = registry.get<MeshCollection>(entity);

            // Update the collider's center position based on the entity's transform
            GW::MATH::GVECTORF worldPos = { 0.0f, 0.0f, 0.0f, 1.0f };
            // Use VectorXMatrixF instead of MultiplyVectorF
            GW::MATH::GMatrix::VectorXMatrixF(transform.matrix, meshCollection.collider.center, worldPos);
            meshCollection.collider.center = worldPos;
        }
    }

    // Handle collision between a bullet and an enemy
    void HandleBulletEnemyCollision(entt::registry& registry, entt::entity bulletEntity, entt::entity enemyEntity) {
        std::cout << "Bullet hit enemy! Destroying both." << std::endl;

        // Destroy both entities
        registry.destroy(bulletEntity);
        registry.destroy(enemyEntity);
    }

    // Handle collision between a player and an enemy
    void HandlePlayerEnemyCollision(entt::registry& registry, entt::entity playerEntity, entt::entity enemyEntity) {
        std::cout << "Player collided with enemy! Game over." << std::endl;

        // For now, just print a message
        // In a real game, we would implement player damage or game over logic here
    }

    // Handle collision between an entity and an obstacle
    void HandleEntityObstacleCollision(entt::registry& registry, entt::entity entityEntity, entt::entity obstacleEntity) {
        // Only handle entities with velocity
        if (!registry.all_of<Velocity>(entityEntity)) {
            return;
        }

        auto& velocity = registry.get<Velocity>(entityEntity);

        // Simple bounce: reverse the velocity
        velocity.direction.x = -velocity.direction.x;
        velocity.direction.z = -velocity.direction.z;

        std::cout << "Entity collided with obstacle! Bouncing." << std::endl;
    }

    // System update function that will be connected to the registry
    void collision_system_update(entt::registry& registry) {
        // Update collider transforms for all entities with Transform and MeshCollection
        auto collidableView = registry.view<Transform, MeshCollection, Collidable>();
        for (auto entity : collidableView) {
            UpdateColliderTransform(registry, entity);
        }

        // Check for collisions between all collidable entities
        for (auto entity1 : collidableView) {
            for (auto entity2 : collidableView) {
                // Skip self-collision
                if (entity1 == entity2) {
                    continue;
                }

                auto& transform1 = registry.get<Transform>(entity1);
                auto& meshCollection1 = registry.get<MeshCollection>(entity1);

                auto& transform2 = registry.get<Transform>(entity2);
                auto& meshCollection2 = registry.get<MeshCollection>(entity2);

                // Check if the entities are colliding
                if (CheckCollision(meshCollection1.collider, transform1.matrix,
                    meshCollection2.collider, transform2.matrix)) {

                    // Handle different types of collisions

                    // Bullet-Enemy collision
                    if (registry.all_of<Bullet>(entity1) && registry.all_of<Enemy>(entity2)) {
                        HandleBulletEnemyCollision(registry, entity1, entity2);
                    }
                    else if (registry.all_of<Enemy>(entity1) && registry.all_of<Bullet>(entity2)) {
                        HandleBulletEnemyCollision(registry, entity2, entity1);
                    }

                    // Player-Enemy collision
                    else if (registry.all_of<Player>(entity1) && registry.all_of<Enemy>(entity2)) {
                        HandlePlayerEnemyCollision(registry, entity1, entity2);
                    }
                    else if (registry.all_of<Enemy>(entity1) && registry.all_of<Player>(entity2)) {
                        HandlePlayerEnemyCollision(registry, entity2, entity1);
                    }

                    // Entity-Obstacle collision
                    else if (registry.all_of<Obstacle>(entity2)) {
                        HandleEntityObstacleCollision(registry, entity1, entity2);
                    }
                    else if (registry.all_of<Obstacle>(entity1)) {
                        HandleEntityObstacleCollision(registry, entity2, entity1);
                    }
                }
            }
        }
    }

    // Initialize the collision system
    void InitializeCollisionSystem(entt::registry& registry) {
        // Create a dummy entity to hold our system
        entt::entity collisionSystem = registry.create();

        // Add a tag component to identify it as a system
        struct CollisionSystemTag {};
        registry.emplace<CollisionSystemTag>(collisionSystem);

        // Connect the system update function to the registry
        registry.on_update<CollisionSystemTag>().connect<&collision_system_update>();

        // Trigger the system to run by patching the component
        registry.patch<CollisionSystemTag>(collisionSystem);

        std::cout << "Collision System initialized" << std::endl;
    }

    GW::MATH::GOBBF TransformOBBToWorldSpace(const GW::MATH::GOBBF& localOBB, const GW::MATH::GMATRIXF& transform)
    {
        GW::MATH::GOBBF worldOBB = localOBB;
        // Scale the extents 
        GW::MATH::GVECTORF scale;

        GW::MATH::GMatrix::GetScaleF(transform, scale);
        worldOBB.extent.x *= scale.x;
        worldOBB.extent.y *= scale.y;
        worldOBB.extent.z *= scale.z;

        // Transform the center 
        GW::MATH::GVECTORF worldCenter;
        GW::MATH::GMatrix::VectorXMatrixF(transform, localOBB.center, worldCenter);
        worldOBB.center = worldCenter;

        // Update the rotation 
        GW::MATH::GQUATERNIONF transformRotation;
        GW::MATH::GMatrix::GetRotationF(transform, transformRotation);

        GW::MATH::GQUATERNIONF combinedRotation;
        combinedRotation.x = localOBB.rotation.w * transformRotation.x + localOBB.rotation.x * transformRotation.w + localOBB.rotation.y * transformRotation.z - localOBB.rotation.z * transformRotation.y;
        combinedRotation.y = localOBB.rotation.w * transformRotation.y - localOBB.rotation.x * transformRotation.z + localOBB.rotation.y * transformRotation.w + localOBB.rotation.z * transformRotation.x;
        combinedRotation.z = localOBB.rotation.w * transformRotation.z + localOBB.rotation.x * transformRotation.y - localOBB.rotation.y * transformRotation.x + localOBB.rotation.z * transformRotation.w;
        combinedRotation.w = localOBB.rotation.w * transformRotation.w - localOBB.rotation.x * transformRotation.x - localOBB.rotation.y * transformRotation.y - localOBB.rotation.z * transformRotation.z;
        worldOBB.rotation = combinedRotation;

        return worldOBB;
    }


    bool CheckOBBCollision(const GW::MATH::GOBBF& obb1, const GW::MATH::GOBBF& obb2) {
        // This is a simplified OBB collision test
        // For now, we'll use a simple distance check between centers
        float distanceX = std::abs(obb1.center.x - obb2.center.x);
        float distanceY = std::abs(obb1.center.y - obb2.center.y);
        float distanceZ = std::abs(obb1.center.z - obb2.center.z);

        // Sum of extents
        float sumExtentX = obb1.extent.x + obb2.extent.x;
        float sumExtentY = obb1.extent.y + obb2.extent.y;
        float sumExtentZ = obb1.extent.z + obb2.extent.z;

        // Check if the OBBs are overlapping in all three axes
    }

    // System update function that will be connected to the registry
    void collision_system_update(entt::registry& registry) {
        // Update collider transforms for all entities with Transform and MeshCollection
        auto collidableView = registry.view<Transform, MeshCollection, Collidable>();
        for (auto entity : collidableView) {
            UpdateColliderTransform(registry, entity);
        }

        // Check for collisions between all collidable entities
        for (auto entity1 : collidableView) {
            for (auto entity2 : collidableView) {
                // Skip self-collision
                if (entity1 == entity2) {
                    continue;
                }

                auto& transform1 = registry.get<Transform>(entity1);
                auto& meshCollection1 = registry.get<MeshCollection>(entity1);

                auto& transform2 = registry.get<Transform>(entity2);
                auto& meshCollection2 = registry.get<MeshCollection>(entity2);

                // Check if the entities are colliding
                if (CheckCollision(meshCollection1.collider, transform1.matrix,
                    meshCollection2.collider, transform2.matrix)) {

                    // Handle different types of collisions

                    // Bullet-Enemy collision
                    if (registry.all_of<Bullet>(entity1) && registry.all_of<Enemy>(entity2)) {
                        HandleBulletEnemyCollision(registry, entity1, entity2);
                    }
                    else if (registry.all_of<Enemy>(entity1) && registry.all_of<Bullet>(entity2)) {
                        HandleBulletEnemyCollision(registry, entity2, entity1);
                    }

                    // Player-Enemy collision
                    else if (registry.all_of<Player>(entity1) && registry.all_of<Enemy>(entity2)) {
                        HandlePlayerEnemyCollision(registry, entity1, entity2);
                    }
                    else if (registry.all_of<Enemy>(entity1) && registry.all_of<Player>(entity2)) {
                        HandlePlayerEnemyCollision(registry, entity2, entity1);
                    }

                    // Entity-Obstacle collision
                    else if (registry.all_of<Obstacle>(entity2)) {
                        HandleEntityObstacleCollision(registry, entity1, entity2);
                    }
                    else if (registry.all_of<Obstacle>(entity1)) {
                        HandleEntityObstacleCollision(registry, entity2, entity1);
                    }
                }
            }
        }
    }

    // Connect the collision system to the registry
    CONNECT_COMPONENT_LOGIC()
    {
        // Initialize the collision system
        GAME::InitializeCollisionSystem(registry);
    }
}
