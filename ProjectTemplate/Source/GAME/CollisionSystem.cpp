#include "CollisionSystem.h"
#include "ModelManager.h"
#include <d3d12.h>
#include <set>
#include <utility>

namespace GAME {

    struct CollisionSystemTag {};

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
        auto& entityTransform = registry.get<Transform>(entityEntity);
        auto& obstacleTransform = registry.get<Transform>(obstacleEntity);

        // Calculate direction from obstacle to entity
        GW::MATH::GVECTORF direction;

        // Get the translation components from the matrices
        GW::MATH::GVECTORF entityPos = { 0.0f, 0.0f, 0.0f, 1.0f };
        GW::MATH::GVECTORF obstaclePos = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Extract the translation from the matrices
        GW::MATH::GMatrix::GetTranslationF(entityTransform.matrix, entityPos);
        GW::MATH::GMatrix::GetTranslationF(obstacleTransform.matrix, obstaclePos);

        // Calculate direction vector
        direction.x = entityPos.x - obstaclePos.x;
        direction.z = entityPos.z - obstaclePos.z;

        // Normalize the direction
        float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
        if (length > 0.0f) {
            direction.x /= length;
            direction.z /= length;
        }
        else {
            // If we can't determine a direction, just reverse the velocity
            direction.x = -velocity.direction.x;
            direction.z = -velocity.direction.z;
        }

        // Set the velocity direction away from the obstacle
        velocity.direction = direction;

        // Move the entity slightly away from the obstacle to prevent getting stuck
        GW::MATH::GVECTORF movement = direction;
        movement.x *= 0.5f; // Increased push to prevent sticking
        movement.z *= 0.5f;
        GW::MATH::GMatrix::TranslateGlobalF(entityTransform.matrix, movement, entityTransform.matrix);

        std::cout << "Entity collided with obstacle! Bouncing with new direction: "
            << direction.x << ", " << direction.z << std::endl;
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
        return (distanceX < sumExtentX && distanceY < sumExtentY && distanceZ < sumExtentZ);
    }

    // Store the collision system entity for later use
    entt::entity g_collisionSystemEntity = entt::null;

    // System update function that will be connected to the registry
    void collision_system_update(entt::registry& registry) {
        // Debug output to confirm the collision system is running
        static int frameCount = 0;
        if (frameCount % 60 == 0) { // Print every 60 frames to avoid console spam
            std::cout << "Collision system update running..." << std::endl;
        }
        frameCount++;

        // Track which entity pairs have already collided in this frame
        std::set<std::pair<entt::entity, entt::entity>> processedCollisions;

        // Get all entities with the Collidable tag
        auto collidableView = registry.view<Collidable, Transform, MeshCollection>();

        // Check for collisions between all collidable entities
        for (auto entity1 : collidableView) {
            auto& transform1 = registry.get<Transform>(entity1);
            auto& meshCollection1 = registry.get<MeshCollection>(entity1);

            // Transform the first OBB to world space
            GW::MATH::GOBBF worldOBB1 = TransformOBBToWorldSpace(meshCollection1.collider, transform1.matrix);

            // Check for collisions with all other collidable entities
            for (auto entity2 : collidableView) {
                // Skip self-collision
                if (entity1 == entity2) {
                    continue;
                }

                // Create a pair of entities (always with the smaller ID first for consistency)
                std::pair<entt::entity, entt::entity> collisionPair;
                if (entity1 < entity2) {
                    collisionPair = std::make_pair(entity1, entity2);
                }
                else {
                    collisionPair = std::make_pair(entity2, entity1);
                }

                // Check if this collision has already been processed
                if (processedCollisions.find(collisionPair) != processedCollisions.end()) {
                    continue; // Skip this collision as it's already been processed
                }

                auto& transform2 = registry.get<Transform>(entity2);
                auto& meshCollection2 = registry.get<MeshCollection>(entity2);

                // Transform the second OBB to world space
                GW::MATH::GOBBF worldOBB2 = TransformOBBToWorldSpace(meshCollection2.collider, transform2.matrix);

                // Check if the OBBs are colliding
                if (CheckOBBCollision(worldOBB1, worldOBB2)) {
                    // Add this collision to the processed set
                    processedCollisions.insert(collisionPair);

                    // Debug output when collision is detected
                    std::cout << "Collision detected between entities " << (int)entity1 << " and " << (int)entity2 << std::endl;

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
        g_collisionSystemEntity = registry.create();

        // Add a tag component to identify it as a system
        registry.emplace<CollisionSystemTag>(g_collisionSystemEntity);

        // Connect the system update function to the registry
        registry.on_update<CollisionSystemTag>().connect<&collision_system_update>();

        std::cout << "Collision System initialized with entity ID: " << (int)g_collisionSystemEntity << std::endl;
    }

    // Function to manually update the collision system
    void UpdateCollisionSystem(entt::registry& registry) {
        if (g_collisionSystemEntity != entt::null && registry.valid(g_collisionSystemEntity)) {
            // Trigger the collision system update by patching the component
            registry.patch<CollisionSystemTag>(g_collisionSystemEntity);
        }
    }

    // Connect the collision system to the registry
    CONNECT_COMPONENT_LOGIC()
    {
        // Initialize the collision system
        GAME::InitializeCollisionSystem(registry);
    }
}