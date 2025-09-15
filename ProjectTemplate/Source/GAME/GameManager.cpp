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
        // Handle keyboard input for toggling visibility 
        HandleVisibilityToggleInput(registry);

        // Update player entities (will use the Player component's on_update method) 
        auto playerView = registry.view<Player>();
        for (auto entity : playerView) {
            registry.patch<Player>(entity); // This will trigger the Player's on_update method 
        }

        // Update entity positions based on velocity
        UpdateVelocitySystem(registry, deltaTime);

        // Check for collisions between entities
        CheckCollisions(registry);

        // Process entities marked for destruction
        ProcessDestroyedEntities(registry);

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
		auto playerEntity = *playerView.begin();
		auto& transform = registry.get<Transform>(playerEntity);

		// Get the GameManager for player speed
		auto& gameManager = registry.ctx().get<GameManager>();
		float speed = gameManager.playerSpeed * deltaTime;

		// Check for keyboard input
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
			GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
			std::cout << "Player moved: " << movement.x << ", " << movement.z << std::endl;
		}
	}

    // Update entity positions based on velocity
    void UpdateVelocitySystem(entt::registry& registry, float deltaTime) {
        // Get all entities with Transform and Velocity components
        auto velocityView = registry.view<Transform, Velocity>();

        // For each entity with Transform and Velocity
        for (auto entity : velocityView) {
            auto& transform = registry.get<Transform>(entity);
            auto& velocity = registry.get<Velocity>(entity);

            // Calculate movement vector based on velocity and delta time
            GW::MATH::GVECTORF movement = {
                velocity.direction.x * velocity.speed * deltaTime,
                velocity.direction.y * velocity.speed * deltaTime,
                velocity.direction.z * velocity.speed * deltaTime,
                0.0f
            };

            // Apply movement to transform
            GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
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

	// Map to store collections of entities by name
	std::map<std::string, std::vector<entt::entity>> modelCollections;

	void AddEntityToCollection(entt::registry& registry, entt::entity entity, const std::string& collectionName) {
		// Get the ModelManager from the registry context
		auto& modelManager = registry.ctx().get<ModelManager>();

		// Add the entity to the collection
		modelManager.collections[collectionName].push_back(entity);
	}

	std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName) {
		// Get the ModelManager from the registry context
		auto& modelManager = registry.ctx().get<ModelManager>();

		// Return the collection if it exists
		if (modelManager.collections.find(collectionName) != modelManager.collections.end()) {
			return modelManager.collections[collectionName];
		}

		// Return an empty vector if the collection doesn't exist
		return std::vector<entt::entity>();
	}

	entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName) {
		// Create a new entity
		entt::entity entity = registry.create();

		// Add a Transform component
		registry.emplace<Transform>(entity);

		// Add a MeshCollection component
		registry.emplace<MeshCollection>(entity);

		// Get the entities from the model collection
		auto modelEntities = GetEntitiesFromCollection(registry, modelName);

		// Add the model entities to the mesh collection
		auto& meshCollection = registry.get<MeshCollection>(entity);
		meshCollection.meshEntities = modelEntities;

		// Add the entity to the model collection
		AddEntityToCollection(registry, entity, modelName);

		return entity;
	}

	// Toggle visibility of an entity
	void ToggleEntityVisibility(entt::registry& registry, entt::entity entity) {
		// Get the mesh collection for this entity
		if (!registry.all_of<MeshCollection>(entity)) {
			return;
		}

		auto& meshCollection = registry.get<MeshCollection>(entity);

		// Toggle DoNotRender tag for each mesh entity
		for (auto meshEntity : meshCollection.meshEntities) {
			if (registry.all_of<DRAW::DoNotRender>(meshEntity)) {
				registry.remove<DRAW::DoNotRender>(meshEntity);
			}
			else {
				registry.emplace<DRAW::DoNotRender>(meshEntity);
			}
		}
	}

	// Set visibility of an entity
	void SetEntityVisibility(entt::registry& registry, entt::entity entity, bool visible) {
		// Get the mesh collection for this entity
		if (!registry.all_of<MeshCollection>(entity)) {
			return;
		}

		auto& meshCollection = registry.get<MeshCollection>(entity);

		// Set DoNotRender tag for each mesh entity based on visibility
		for (auto meshEntity : meshCollection.meshEntities) {
			if (visible) {
				if (registry.all_of<DRAW::DoNotRender>(meshEntity)) {
					registry.remove<DRAW::DoNotRender>(meshEntity);
				}
			}
			else {
				if (!registry.all_of<DRAW::DoNotRender>(meshEntity)) {
					registry.emplace<DRAW::DoNotRender>(meshEntity);
				}
			}
		}
	}

	// Handle keyboard input for toggling visibility
	void HandleVisibilityToggleInput(entt::registry& registry) {
		// Get the input from the registry context
		auto& input = registry.ctx().get<UTIL::Input>();
		auto& gameManager = registry.ctx().get<GameManager>();

		// Check for P key press to toggle player visibility
		float pKey = 0.0f;
		static bool pKeyPressed = false;
		input.immediateInput.GetState(G_KEY_P, pKey);

		if (pKey > 0.0f && !pKeyPressed) {
			pKeyPressed = true;
			gameManager.playerVisible = !gameManager.playerVisible;

			// Find the player entity
			auto playerView = registry.view<Player>();
			if (playerView.begin() != playerView.end()) {
				auto playerEntity = *playerView.begin();
				SetEntityVisibility(registry, playerEntity, gameManager.playerVisible);
				std::cout << "Player visibility toggled: " << (gameManager.playerVisible ? "visible" : "hidden") << std::endl;
			}
		}
		else if (pKey <= 0.0f) {
			pKeyPressed = false;
		}

		// Check for E key press to toggle enemy visibility
		float eKey = 0.0f;
		static bool eKeyPressed = false;
		input.immediateInput.GetState(G_KEY_E, eKey);

		if (eKey > 0.0f && !eKeyPressed) {
			eKeyPressed = true;
			gameManager.enemyVisible = !gameManager.enemyVisible;

			// Find the enemy entity
			auto enemyView = registry.view<Enemy>();
			if (enemyView.begin() != enemyView.end()) {
				auto enemyEntity = *enemyView.begin();
				SetEntityVisibility(registry, enemyEntity, gameManager.enemyVisible);
				std::cout << "Enemy visibility toggled: " << (gameManager.enemyVisible ? "visible" : "hidden") << std::endl;
			}
		}
		else if (eKey <= 0.0f) {
			eKeyPressed = false;
		}
	}

    // Check for collisions between entities
    void CheckCollisions(entt::registry& registry) {
        // Get all entities with Transform, MeshCollection, and Collidable components
        auto collidableView = registry.view<Transform, MeshCollection, Collidable>();

        // For each collidable entity
        for (auto entity1 : collidableView) {
            auto& transform1 = registry.get<Transform>(entity1);
            auto& meshCollection1 = registry.get<MeshCollection>(entity1);

            // Check against all other collidable entities
            for (auto entity2 : collidableView) {
                // Skip self-collision
                if (entity1 == entity2) continue;

                auto& transform2 = registry.get<Transform>(entity2);
                auto& meshCollection2 = registry.get<MeshCollection>(entity2);

                // Check for collision between the two entities' OBBs
                GW::MATH::GOBBF obb1 = meshCollection1.obb;
                GW::MATH::GOBBF obb2 = meshCollection2.obb;

                // Transform OBBs by entity transforms
                GW::MATH::GOBBF transformedObb1, transformedObb2;
                GW::MATH::GMatrix::MultiplyMatrixF(obb1.orientation, transform1.matrix, transformedObb1.orientation);
                GW::MATH::GMatrix::MultiplyMatrixF(obb2.orientation, transform2.matrix, transformedObb2.orientation);
                
                // Update centers
                GW::MATH::GMatrix::TransformCoordF(obb1.center, transform1.matrix, transformedObb1.center);
                GW::MATH::GMatrix::TransformCoordF(obb2.center, transform2.matrix, transformedObb2.center);

                // Check for collision
                bool collision = false;
                // TODO: Implement proper OBB-OBB collision detection
                // For now, we'll use a simple distance check as placeholder
                float distance = std::sqrt(
                    std::pow(transformedObb1.center.x - transformedObb2.center.x, 2) +
                    std::pow(transformedObb1.center.y - transformedObb2.center.y, 2) +
                    std::pow(transformedObb1.center.z - transformedObb2.center.z, 2)
                );

                // Simple collision detection based on distance
                float collisionThreshold = 1.0f; // Adjust as needed
                collision = distance < collisionThreshold;

                if (collision) {
                    // Handle collision based on entity types
                    bool isEntity1Bullet = registry.all_of<Bullet>(entity1);
                    bool isEntity2Bullet = registry.all_of<Bullet>(entity2);
                    bool isEntity1Enemy = registry.all_of<Enemy>(entity1);
                    bool isEntity2Enemy = registry.all_of<Enemy>(entity2);
                    bool isEntity1Obstacle = registry.all_of<Obstacle>(entity1);
                    bool isEntity2Obstacle = registry.all_of<Obstacle>(entity2);

                    // Bullet hits enemy
                    if ((isEntity1Bullet && isEntity2Enemy) || (isEntity2Bullet && isEntity1Enemy)) {
                        auto bulletEntity = isEntity1Bullet ? entity1 : entity2;
                        auto enemyEntity = isEntity1Enemy ? entity1 : entity2;

                        // Mark bullet for destruction
                        registry.emplace<ToDestroy>(bulletEntity);

                        // Handle enemy hit logic
                        if (registry.all_of<Shatters>(enemyEntity)) {
                            auto& shatters = registry.get<Shatters>(enemyEntity);
                            if (shatters.shatterCount > 0) {
                                // Get enemy transform and velocity
                                auto& enemyTransform = registry.get<Transform>(enemyEntity);
                                GW::MATH::GVECTORF enemyPosition;
                                GW::MATH::GMatrix::GetTranslationF(enemyTransform.matrix, enemyPosition);
                                
                                // Only get velocity if it exists
                                GW::MATH::GVECTORF enemyDirection = { 0.0f, 0.0f, 0.0f };
                                float enemySpeed = 0.0f;
                                if (registry.all_of<Velocity>(enemyEntity)) {
                                    auto& enemyVelocity = registry.get<Velocity>(enemyEntity);
                                    enemyDirection = enemyVelocity.direction;
                                    enemySpeed = enemyVelocity.speed;
                                }
                                
                                // Create shattered pieces
                                for (int i = 0; i < shatters.shatterAmount; i++) {
                                    // Create a new enemy entity
                                    entt::entity newEnemyEntity = CreateGameEntityFromModel(registry, "Cactus");
                                    
                                    // Add Enemy tag
                                    registry.emplace<Enemy>(newEnemyEntity);
                                    
                                    // Add Collidable tag
                                    registry.emplace<Collidable>(newEnemyEntity);
                                    
                                    // Add Shatters component with reduced count
                                    registry.emplace<Shatters>(newEnemyEntity, 
                                                              shatters.shatterCount - 1, 
                                                              shatters.shatterAmount, 
                                                              shatters.shatterScale);
                                    
                                    // Scale down the new enemy
                                    auto& newTransform = registry.get<Transform>(newEnemyEntity);
                                    GW::MATH::GMATRIXF scaleMatrix;
                                    GW::MATH::GMatrix::ScalingF(scaleMatrix, 
                                                               shatters.shatterScale, 
                                                               shatters.shatterScale, 
                                                               shatters.shatterScale);
                                    GW::MATH::GMatrix::MultiplyMatrixF(scaleMatrix, newTransform.matrix, newTransform.matrix);
                                    
                                    // Position the new enemy near the original enemy with slight offset
                                    GW::MATH::GVECTORF offset = {
                                        ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.5f, // -0.5 to 0.5
                                        0.0f,
                                        ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.5f  // -0.5 to 0.5
                                    };
                                    
                                    GW::MATH::GVECTORF newPosition = {
                                        enemyPosition.x + offset.x,
                                        enemyPosition.y + offset.y,
                                        enemyPosition.z + offset.z
                                    };
                                    
                                    GW::MATH::GMatrix::TranslateGlobalF(newTransform.matrix, newPosition, newTransform.matrix);
                                    
                                    // Create a new random direction for the enemy
                                    GW::MATH::GVECTORF newDirection = { 0.0f, 0.0f, 0.0f };
                                    
                                    // Generate random direction ensuring it's diagonal (both x and z are non-zero)
                                    do {
                                        newDirection.x = (float)rand() / RAND_MAX * 2.0f - 1.0f; // -1 to 1
                                        newDirection.z = (float)rand() / RAND_MAX * 2.0f - 1.0f; // -1 to 1
                                    } while (std::abs(newDirection.x) < 0.3f || std::abs(newDirection.z) < 0.3f);
                                    
                                    // Normalize the direction vector
                                    float length = std::sqrt(newDirection.x * newDirection.x + newDirection.z * newDirection.z);
                                    newDirection.x /= length;
                                    newDirection.z /= length;
                                    
                                    // Add the Velocity component with slightly increased speed
                                    registry.emplace<Velocity>(newEnemyEntity, newDirection, enemySpeed * 1.2f);
                                    
                                    std::cout << "Created shattered enemy piece " << i+1 << " of " << shatters.shatterAmount << std::endl;
                                }
                                
                                // Mark the original enemy for destruction
                                registry.emplace<ToDestroy>(enemyEntity);
                                std::cout << "Enemy shattered into " << shatters.shatterAmount << " pieces" << std::endl;
                            } else {
                                // If no more shatters left, just destroy the enemy
                                registry.emplace<ToDestroy>(enemyEntity);
                                std::cout << "Enemy destroyed (no more shatters)" << std::endl;
                            }
                        } else {
                            // If enemy doesn't have Shatters component, just destroy it
                            registry.emplace<ToDestroy>(enemyEntity);
                            std::cout << "Enemy destroyed (no shatters component)" << std::endl;
                        }
                    }
                    // Bullet hits obstacle
                    else if ((isEntity1Bullet && isEntity2Obstacle) || (isEntity2Bullet && isEntity1Obstacle)) {
                        auto bulletEntity = isEntity1Bullet ? entity1 : entity2;
                        // Mark bullet for destruction
                        registry.emplace<ToDestroy>(bulletEntity);
                    }
                    // Enemy hits obstacle
                    else if ((isEntity1Enemy && isEntity2Obstacle) || (isEntity2Enemy && isEntity1Obstacle)) {
                        auto enemyEntity = isEntity1Enemy ? entity1 : entity2;
                        
                        // Bounce the enemy off the obstacle
                        if (registry.all_of<Velocity>(enemyEntity)) {
                            auto& velocity = registry.get<Velocity>(enemyEntity);
                            
                            // Simple bounce: invert velocity components
                            velocity.direction.x = -velocity.direction.x;
                            velocity.direction.z = -velocity.direction.z;
                        }
                    }
                }
            }
        }
    }

    // Process entities marked for destruction
    void ProcessDestroyedEntities(entt::registry& registry) {
        // Get all entities marked for destruction
        auto destroyView = registry.view<ToDestroy>();

        // Destroy each entity
        for (auto entity : destroyView) {
            // If the entity has a MeshCollection, destroy all mesh entities
            if (registry.all_of<MeshCollection>(entity)) {
                auto& meshCollection = registry.get<MeshCollection>(entity);
                for (auto meshEntity : meshCollection.meshEntities) {
                    registry.destroy(meshEntity);
                }
            }
            
            // Destroy the entity itself
            registry.destroy(entity);
        }
    }

	// on_update method for the GameManager component
	void on_update(entt::registry& registry, entt::entity entity) {
		// Get the delta time from the registry context
		auto& deltaTime = registry.ctx().get<UTIL::DeltaTime>().dtSec;

		// Update the GameManager
		UpdateGameManager(registry, static_cast<float>(deltaTime));
	}

	// Connect the GameManager logic to the registry
	CONNECT_COMPONENT_LOGIC() {
		// Initialize the GameManager
		InitializeGameManager(registry);

		// Connect the on_update method
		registry.on_update<GameManager>().connect<&on_update>();
	}

} // namespace GAME