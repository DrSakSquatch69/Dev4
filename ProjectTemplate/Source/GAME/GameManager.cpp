#include "GameManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"

namespace GAME {

	void InitializeGameManager(entt::registry& registry) {
		// Create a GameManager in the registry context
		registry.ctx().emplace<GameManager>();
		std::cout << "GameManager initialized" << std::endl;
	}

	// Update entity transforms based on velocity
	void UpdateVelocitySystem(entt::registry& registry, float deltaTime) {
		// Get all entities with Transform and Velocity components
		auto velocityView = registry.view<Transform, Velocity>();

		// Update each entity's position based on its velocity
		for (auto entity : velocityView) {
			auto& transform = registry.get<Transform>(entity);
			auto& velocity = registry.get<Velocity>(entity);

			// Calculate movement vector based on velocity direction and speed
			GW::MATH::GVECTORF movement = velocity.direction;

			// Scale by speed and deltaTime
			movement.x *= velocity.speed * deltaTime;
			movement.y *= velocity.speed * deltaTime;
			movement.z *= velocity.speed * deltaTime;

			// Apply movement to transform
			GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
		}
	}
    
	void UpdateGameManager(entt::registry& registry, float deltaTime) {
        // Get the GameManager from the registry context
        auto& gameManager = registry.ctx().get<GameManager>();

		// Update entity transforms based on velocity (new system)
		UpdateVelocitySystem(registry, deltaTime);

        // Handle keyboard input for toggling visibility 
        HandleVisibilityToggleInput(registry);

        // Update player entities (will use the Player component's on_update method) 
        auto playerView = registry.view<Player>();
        for (auto entity : playerView) {
            registry.patch<Player>(entity); // This will trigger the Player's on_update method 
        }
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
		modelCollections[collectionName].push_back(entity);
	}

	std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName) {
		if (modelCollections.find(collectionName) != modelCollections.end()) {
			return modelCollections[collectionName];
		}
		return std::vector<entt::entity>();
	}

	entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName) {
		// Create the entity
		entt::entity gameEntity = registry.create();

		// Add a MeshCollection component
		registry.emplace<MeshCollection>(gameEntity);

		// Add a Transform component with identity matrix initially
		auto& transform = registry.emplace<Transform>(gameEntity);
		GW::MATH::GMatrix::IdentityF(transform.matrix);

		// Get entities from the model collection
		auto modelEntities = GetEntitiesFromCollection(registry, modelName);
		std::cout << "Model collection " << modelName << " has " << modelEntities.size() << " entities" << std::endl;

		// For each entity in the model collection
		for (auto modelEntity : modelEntities) {
			// Create a new entity for the mesh
			entt::entity meshEntity = registry.create();

			// Copy the GeometryData and GPUInstance components
			if (registry.all_of<DRAW::GeometryData>(modelEntity)) {
				auto& geomData = registry.get<DRAW::GeometryData>(modelEntity);
				registry.emplace<DRAW::GeometryData>(meshEntity, geomData);
			}

			if (registry.all_of<DRAW::GPUInstance>(modelEntity)) {
				auto& gpuInstance = registry.get<DRAW::GPUInstance>(modelEntity);
				registry.emplace<DRAW::GPUInstance>(meshEntity, gpuInstance);

				if (modelEntities[0] == modelEntity) {
					transform.matrix = gpuInstance.transform; // Copy the entire transform
				}
			}

			// Add the mesh entity to the game entity's MeshCollection
			auto& meshCollection = registry.get<MeshCollection>(gameEntity);
			meshCollection.meshEntities.push_back(meshEntity);
		}

		return gameEntity;
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