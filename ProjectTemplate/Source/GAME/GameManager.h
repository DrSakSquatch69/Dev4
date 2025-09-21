#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"
#include "ModelManager.h"
#include "GameComponents.h"

namespace GAME
{
    // GameManager component to store game state
    struct GameManager {
        float playerSpeed = 5.0f; // Units per second
        bool playerVisible = true; // Flag to control player visibility
        bool enemyVisible = true;  // Flag to control enemy visibility
    };

    // Initialize the GameManager
    void InitializeGameManager(entt::registry& registry);

    // Update the GameManager
    void UpdateGameManager(entt::registry& registry, float deltaTime);
    // Update velocity system for moving entities
    void UpdateVelocitySystem(entt::registry& registry, float deltaTime);

    // on_update method for the GameManager component
    void on_update(entt::registry& registry, entt::entity entity);

    // Update player movement based on input
    void UpdatePlayerMovement(entt::registry& registry, float deltaTime);

    // Update GPU instances from Transform components
    void UpdateGPUInstances(entt::registry& registry);

    // Create wall entities from level data
    void CreateWalls(entt::registry& registry);
    
    // Adjust wall collider based on position
    void AdjustWallCollider(entt::registry& registry, entt::entity wallEntity, const GW::MATH::GMATRIXF& transform, const std::string& wallName);

    // Check for collisions between collidable entities
    void CheckCollisions(entt::registry& registry);

    // Handle collision between two entities
    void HandleCollision(entt::registry& registry, entt::entity entity1, entt::entity entity2);

    // Check if two entities are colliding
    bool AreEntitiesColliding(entt::registry& registry, entt::entity entity1, entt::entity entity2);

    // Add an entity to a named collection
    void AddEntityToCollection(entt::registry& registry, entt::entity entity, const std::string& collectionName);

    // Get entities from a named collection
    std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName);

    // Create a game entity from a model
    entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName);

    // Toggle visibility of an entity
    void ToggleEntityVisibility(entt::registry& registry, entt::entity entity);

    // Set visibility of an entity
    void SetEntityVisibility(entt::registry& registry, entt::entity entity, bool visible);

    // Handle keyboard input for toggling visibility
    void HandleVisibilityToggleInput(entt::registry& registry);
    
    // Handle enemy shattering when hit by bullets
    void HandleEnemyShattering(entt::registry& registry, entt::entity enemyEntity);
}

#endif // GAME_MANAGER_H