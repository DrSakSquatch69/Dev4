#ifndef GAME_MANAGER_H_
#define GAME_MANAGER_H_

#include "../../entt-3.13.1/single_include/entt/entt.hpp"
#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME {
    // GameManager component to manage game state
    struct GameManager {
        // Add any game state variables here
        float playerSpeed = 8.0f; // Default player speed from defaults.ini
    };

    // Initialize the GameManager
    void InitializeGameManager(entt::registry& registry);

    // Update the GameManager
    void UpdateGameManager(entt::registry& registry, float deltaTime);

    // Update player movement based on input
    void UpdatePlayerMovement(entt::registry& registry, float deltaTime);

    // Update GPUInstances from Transform components
    void UpdateGPUInstances(entt::registry& registry);

} // namespace GAME

#endif // !GAME_MANAGER_H_