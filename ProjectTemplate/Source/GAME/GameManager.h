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

    // on_update method for the GameManager component
    void on_update(entt::registry& registry, entt::entity entity);

    // Update player movement based on input
    void UpdatePlayerMovement(entt::registry& registry, float deltaTime);

    // Update GPU instances from Transform components
    void UpdateGPUInstances(entt::registry& registry);

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
}

#endif // GAME_MANAGER_H