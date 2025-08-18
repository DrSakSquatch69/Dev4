#include "GameManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"

namespace GAME {

    void InitializeGameManager(entt::registry& registry) {
        // Create a GameManager in the registry context
        registry.ctx().emplace<GameManager>();
        std::cout << "GameManager initialized" << std::endl;
    }

    void UpdateGameManager(entt::registry& registry, float deltaTime) {
        // Get the GameManager from the registry context
        auto& gameManager = registry.ctx().get<GameManager>();

        // Update player movement based on input
        UpdatePlayerMovement(registry, deltaTime);

        // Update GPU instances from Transform components
        UpdateGPUInstances(registry);
    }

    void UpdatePlayerMovement(entt::registry& registry, float deltaTime) {
        // Get the input from the registry context
        auto& input = registry.ctx().get<UTIL::Input>();

        // Find the player entity
        auto playerView = registry.view<Player, Transform>();
        if (playerView.begin() == playerView.end()) {
            std::cout << "No player entity found" << std::endl;
            return;
        }

        // Get the player entity and its transform
        auto playerEntity = playerView.front();
        auto& transform = registry.get<Transform>(playerEntity);

        // Get the GameManager for player speed
        auto& gameManager = registry.ctx().get<GameManager>();
        float speed = gameManager.playerSpeed * deltaTime;

        // Check for keyboard input
        unsigned char keyBuffer[256];
        // Fix for GetState - it requires a key code and an output float
        float keyState = 0.0f;

        // Check arrow keys for movement using individual GetState calls
        float rightKey = 0.0f, leftKey = 0.0f, upKey = 0.0f, downKey = 0.0f;
        input.immediateInput.GetState(G_KEY_RIGHT, rightKey);
        input.immediateInput.GetState(G_KEY_LEFT, leftKey);
        input.immediateInput.GetState(G_KEY_UP, upKey);
        input.immediateInput.GetState(G_KEY_DOWN, downKey);

        // Movement vectors
        GW::MATH::GVECTORF movement = { 0.0f, 0.0f, 0.0f };

        // Check arrow keys for movement using the key states we retrieved
        if (rightKey > 0.0f) {
            movement.x += speed;
        }
        if (leftKey > 0.0f) {
            movement.x -= speed;
        }
        if (upKey > 0.0f) {
            movement.z += speed;
        }
        if (downKey > 0.0f) {
            movement.z -= speed;
        }

        // Apply movement to transform
        if (movement.x != 0.0f || movement.z != 0.0f) {
            GW::MATH::GMATRIXF translation;
            GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
            std::cout << "Player moved: " << movement.x << ", " << movement.z << std::endl;
        }
    }

    void UpdateGPUInstances(entt::registry& registry) {
        // Get all entities with Transform and MeshCollection components
        auto transformView = registry.view<Transform, MeshCollection>();

        // For each entity with Transform and MeshCollection
        for (auto entity : transformView) {
            auto& transform = registry.get<Transform>(entity);
            auto& meshCollection = registry.get<MeshCollection>(entity);

            // Update the transform of each mesh in the collection
            for (auto meshEntity : meshCollection.meshEntities) {
                if (registry.all_of<DRAW::GPUInstance>(meshEntity)) {
                    auto& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
                    gpuInstance.transform = transform.matrix;


                }
            }
        }
    }

    // Connect the GameManager logic to the registry
    CONNECT_COMPONENT_LOGIC() {
        // Initialize the GameManager
        InitializeGameManager(registry);
    }

} // namespace GAME