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
		// Update entities with velocity
		UpdateVelocitySystem(registry, deltaTime);

		// Check for collisions
		CheckCollisions(registry);

		// Remove destroyed entities
		RemoveDestroyedEntities(registry);
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

	void UpdateVelocitySystem(entt::registry& registry, float deltaTime) {
		// Get all entities with Transform and Velocity components
		auto velocityView = registry.view<Transform, Velocity>();

		// For each entity with Transform and Velocity
		for (auto entity : velocityView) {
			auto& transform = velocityView.get<Transform>(entity);
			const auto& velocity = velocityView.get<Velocity>(entity);

			// Calculate movement vector based on velocity and delta time
			GW::MATH::GVECTORF movement;
			movement.x = velocity.direction.x * velocity.speed * deltaTime;
			movement.y = velocity.direction.y * velocity.speed * deltaTime;
			movement.z = velocity.direction.z * velocity.speed * deltaTime;

			// Apply movement to transform
			GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
		}
	}

	void CheckCollisions(entt::registry& registry) {
		// Get all entities with Transform, MeshCollection, and Collidable components
		auto collidableView = registry.view<Transform, MeshCollection, Collidable>();

		// For each collidable entity
		for (auto entity1 : collidableView) {
			auto& transform1 = collidableView.get<Transform>(entity1);

			// Get the entity's position
			GW::MATH::GVECTORF position1;
			position1.x = transform1.matrix.row4.x;
			position1.y = transform1.matrix.row4.y;
			position1.z = transform1.matrix.row4.z;

			// Check against all other collidable entities
			for (auto entity2 : collidableView) {
				// Skip self-collision
				if (entity1 == entity2) continue;

				auto& transform2 = collidableView.get<Transform>(entity2);

				// Get the other entity's position
				GW::MATH::GVECTORF position2;
				position2.x = transform2.matrix.row4.x;
				position2.y = transform2.matrix.row4.y;
				position2.z = transform2.matrix.row4.z;

				// Calculate distance between entities
				float dx = position1.x - position2.x;
				float dy = position1.y - position2.y;
				float dz = position1.z - position2.z;
				float distanceSquared = dx * dx + dy * dy + dz * dz;

				// Simple sphere collision detection
				// Assuming a collision radius of 1.0 for now
				float collisionRadiusSum = 1.0f;
				if (distanceSquared < collisionRadiusSum * collisionRadiusSum) {
					// Collision detected!
					HandleCollision(registry, entity1, entity2);
				}
			}
		}
	}

	void HandleCollision(entt::registry& registry, entt::entity entity1, entt::entity entity2) {
		// Check if either entity is an enemy
		bool isEntity1Enemy = registry.all_of<Enemy>(entity1);
		bool isEntity2Enemy = registry.all_of<Enemy>(entity2);

		// Check if either entity is a bullet
		bool isEntity1Bullet = registry.all_of<Bullet>(entity1);
		bool isEntity2Bullet = registry.all_of<Bullet>(entity2);

		// Enemy-Enemy collision: bounce off each other
		if (isEntity1Enemy && isEntity2Enemy) {
			if (registry.all_of<Velocity>(entity1) && registry.all_of<Velocity>(entity2)) {
				auto& velocity1 = registry.get<Velocity>(entity1);
				auto& velocity2 = registry.get<Velocity>(entity2);

				// Simple bounce: reverse directions
				velocity1.direction.x = -velocity1.direction.x;
				velocity1.direction.z = -velocity1.direction.z;

				velocity2.direction.x = -velocity2.direction.x;
				velocity2.direction.z = -velocity2.direction.z;
			}
		}
		// Enemy-Bullet collision: destroy bullet, handle enemy shattering
		else if ((isEntity1Enemy && isEntity2Bullet) || (isEntity1Bullet && isEntity2Bullet)) {
			entt::entity enemyEntity = isEntity1Enemy ? entity1 : entity2;
			entt::entity bulletEntity = isEntity1Bullet ? entity1 : entity2;

			// Mark the bullet for destruction
			registry.emplace<ToDestroy>(bulletEntity);

			// Handle enemy shattering (will be implemented in Part 4)
			if (registry.all_of<Shatters>(enemyEntity)) {
				HandleEnemyShattering(registry, enemyEntity);
			}
		}
		// For other collisions (e.g., with walls), we could add more logic here
	}

	void RemoveDestroyedEntities(entt::registry& registry) {
		// Get all entities marked for destruction
		auto destroyView = registry.view<ToDestroy>();

		// Destroy each entity
		for (auto entity : destroyView) {
			registry.destroy(entity);
		}
	}

	void HandleEnemyShattering(entt::registry& registry, entt::entity enemyEntity) {
		// Get the Shatters component
		auto& shatters = registry.get<Shatters>(enemyEntity);

		// If the enemy can still shatter
		if (shatters.count > 0) {
			// Get the enemy's transform
			auto& enemyTransform = registry.get<Transform>(enemyEntity);

			// Get the enemy's position
			GW::MATH::GVECTORF position;
			position.x = enemyTransform.matrix.row4.x;
			position.y = enemyTransform.matrix.row4.y;
			position.z = enemyTransform.matrix.row4.z;

			// Create smaller enemy pieces
			for (int i = 0; i < shatters.amount; i++) {
				// Create a new enemy entity
				entt::entity pieceEntity = CreateGameEntityFromModel(registry, "Enemy");

				// Add the Enemy tag
				registry.emplace<Enemy>(pieceEntity);

				// Add a Collidable tag
				registry.emplace<Collidable>(pieceEntity);

				// Add a Shatters component with reduced count
				registry.emplace<Shatters>(pieceEntity, shatters.count - 1, shatters.amount, shatters.scale);

				// Get the piece's transform
				auto& pieceTransform = registry.get<Transform>(pieceEntity);

				// Scale the piece
				GW::MATH::GMatrix::ScalingF(pieceTransform.matrix, shatters.scale, shatters.scale, shatters.scale, pieceTransform.matrix);

				// Position the piece near the original enemy
				float offsetX = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.5f;
				float offsetZ = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.5f;
				GW::MATH::GMatrix::TranslationF(pieceTransform.matrix,
					position.x + offsetX,
					position.y,
					position.z + offsetZ,
					pieceTransform.matrix);

				// Add a random velocity
				GW::MATH::GVECTORF direction = UTIL::GetRandomDiagonalDirection();
				registry.emplace<Velocity>(pieceEntity, direction, 5.0f); // Faster than the original enemy
			}

			// Mark the original enemy for destruction
			registry.emplace<ToDestroy>(enemyEntity);
		}
		else {
			// If the enemy can't shatter anymore, just mark it for destruction
			registry.emplace<ToDestroy>(enemyEntity);
		}
	}

	// Connect the GameManager logic to the registry
	CONNECT_COMPONENT_LOGIC() {
		// Initialize the GameManager
		InitializeGameManager(registry);

		// Connect the on_update method
		registry.on_update<GameManager>().connect<&on_update>();
	}

} // namespace GAME