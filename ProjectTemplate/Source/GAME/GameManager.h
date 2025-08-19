#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME
{
    // Tags for game entities
    struct Player {};
    struct Enemy {};
    struct Bullet {};

    // Transform component for game entities
    struct Transform {
        GW::MATH::GMATRIXF matrix;
    };

    // Collection of mesh entities that make up a game entity
    struct MeshCollection {
        std::vector<entt::entity> meshEntities;
    };

    // GameManager component to store game state
    struct GameManager {
        float playerSpeed = 5.0f; // Units per second
    };

    // Initialize the GameManager
    void InitializeGameManager(entt::registry& registry);

    // Update the GameManager
    void UpdateGameManager(entt::registry& registry, float deltaTime);

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
}

#endif // GAME_MANAGER_H