#include "GameManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"
#include <vector>
#include <set>
#include <algorithm>
using namespace GAME;

namespace GAME {

	void InitializeGameManager(entt::registry& registry) {
		// Create a GameManager in the registry context
		registry.ctx().emplace<GameManager>();
		std::cout << "GameManager initialized" << std::endl;
	}

	void UpdateVelocitySystem(entt::registry& registry, float deltaTime) {
		// Get all entities with Transform and Velocity components
		auto velocityView = registry.view<Transform, Velocity>();

		// For each entity with velocity
		for (auto entity : velocityView) {
			// Get the transform and velocity components
			auto& transform = registry.get<Transform>(entity);
			auto& velocity = registry.get<Velocity>(entity);

			// Calculate movement based on velocity and delta time
			GW::MATH::GVECTORF movement = {
				velocity.direction.x * velocity.speed * deltaTime,
				velocity.direction.y * velocity.speed * deltaTime,
				velocity.direction.z * velocity.speed * deltaTime
			};

			// Apply movement to transform
			GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);

			// Debug output
			if (registry.all_of<Bullet>(entity)) {
				std::cout << "Bullet moved: " << movement.x << ", " << movement.z << std::endl;
			}
		}
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

		// Update positions based on velocity
		UpdateVelocitySystem(registry, deltaTime);

		// Check for collisions between entities
		CheckCollisions(registry);

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

	void CreateWalls(entt::registry& registry) {
		// Find all static, collidable objects in the level data
		auto levelView = registry.view<DRAW::CPULevel>();
		if (levelView.empty()) {
			std::cout << "No level data found" << std::endl;
			return;
		}

		auto levelEntity = *levelView.begin();
		auto& cpuLevel = registry.get<DRAW::CPULevel>(levelEntity);

		// Iterate through all blender objects in the level
		for (const auto& blenderObj : cpuLevel.lvlData.blenderObjects) {
			// Get the model for this object
			if (blenderObj.modelIndex >= cpuLevel.lvlData.levelModels.size()) continue;
			const auto& model = cpuLevel.lvlData.levelModels[blenderObj.modelIndex];

			// Check if this model is static and collidable (potential wall)
			if (!model.isDynamic && model.isCollidable) {
				// Create a game entity for this wall
				std::string modelName = model.filename;
				std::string collectionName = modelName;
				size_t lastSlash = collectionName.find_last_of("/\\");
				if (lastSlash != std::string::npos)
					collectionName = collectionName.substr(lastSlash + 1);

				size_t lastDot = collectionName.find_last_of(".");
				if (lastDot != std::string::npos)
					collectionName = collectionName.substr(0, lastDot);

				// Create a wall entity
				entt::entity wallEntity = CreateGameEntityFromModel(registry, collectionName);

				// Add Wall and Collidable tags
				registry.emplace<Wall>(wallEntity);
				registry.emplace<Collidable>(wallEntity);

				// Set the wall's position based on the transform from the level data
				auto& transform = registry.get<Transform>(wallEntity);
				if (blenderObj.transformIndex < cpuLevel.lvlData.levelTransforms.size())
					transform.matrix = cpuLevel.lvlData.levelTransforms[blenderObj.transformIndex];

				// Get wall position and scale
				GW::MATH::GVECTORF wallPos;
				GW::MATH::GMatrix::GetTranslationF(transform.matrix, wallPos);

				GW::MATH::GVECTORF wallScale;
				GW::MATH::GMatrix::GetScaleF(transform.matrix, wallScale);

				// Set up the wall's collider directly based on its position
				auto& meshCollection = registry.get<MeshCollection>(wallEntity);

				// Set the collider center to the origin (relative to the wall's position)
				meshCollection.collider.center = { 0.0f, 0.0f, 0.0f };

				// Set the collider extents based on the wall's position and scale
				if (wallPos.x < -15.0f) {
					// Left wall (negative X)
					std::cout << "CREATING LEFT WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						0.5f,                // Half thickness (X)
						wallScale.y * 5.0f,  // Half height (Y) - assuming the wall is about 10 units tall
						wallScale.z * 10.0f  // Half length (Z) - assuming the wall is about 20 units long
					};
				}
				else if (wallPos.x > 15.0f) {
					// Right wall (positive X)
					std::cout << "CREATING RIGHT WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						0.5f,                // Half thickness (X)
						wallScale.y * 5.0f,  // Half height (Y)
						wallScale.z * 10.0f  // Half length (Z)
					};
				}
				else if (wallPos.z > 15.0f) {
					// Top wall (positive Z)
					std::cout << "CREATING TOP WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						wallScale.x * 10.0f, // Half width (X)
						wallScale.y * 5.0f,  // Half height (Y)
						0.5f                 // Half thickness (Z)
					};
				}
				else if (wallPos.z < -15.0f) {
					// Bottom wall (negative Z)
					std::cout << "CREATING BOTTOM WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						wallScale.x * 10.0f, // Half width (X)
						wallScale.y * 5.0f,  // Half height (Y)
						0.5f                 // Half thickness (Z)
					};
				}
				else {
					// If it's not one of the main walls, use the collider from the level data
					if (model.colliderIndex < cpuLevel.lvlData.levelColliders.size()) {
						meshCollection.collider = cpuLevel.lvlData.levelColliders[model.colliderIndex];
					}
				}

				// Debug output for collider
				std::cout << "WALL DEBUG: " << collectionName
					<< " at position (" << wallPos.x << ", " << wallPos.y << ", " << wallPos.z << ")"
					<< ", collider center: (" << meshCollection.collider.center.x
					<< ", " << meshCollection.collider.center.y
					<< ", " << meshCollection.collider.center.z << ")"
					<< ", extents: (" << meshCollection.collider.extent.x
					<< ", " << meshCollection.collider.extent.y
					<< ", " << meshCollection.collider.extent.z << ")" << std::endl;

				// Debug output for wall transform
				std::cout << "WALL TRANSFORM: " << collectionName
					<< " scale: (" << wallScale.x << ", " << wallScale.y << ", " << wallScale.z << ")" << std::endl;

				// Print the full transform matrix for debugging
				std::cout << "WALL MATRIX: " << collectionName << std::endl;
				for (int i = 0; i < 4; i++) {
					std::cout << "  [";
					for (int j = 0; j < 4; j++) {
						std::cout << transform.matrix.data[i * 4 + j];
						if (j < 3) std::cout << ", ";
					}
					std::cout << "]" << std::endl;
				}

				// Calculate world-space collider boundaries
				float minX = wallPos.x + meshCollection.collider.center.x - meshCollection.collider.extent.x;
				float maxX = wallPos.x + meshCollection.collider.center.x + meshCollection.collider.extent.x;
				float minY = wallPos.y + meshCollection.collider.center.y - meshCollection.collider.extent.y;
				float maxY = wallPos.y + meshCollection.collider.center.y + meshCollection.collider.extent.y;
				float minZ = wallPos.z + meshCollection.collider.center.z - meshCollection.collider.extent.z;
				float maxZ = wallPos.z + meshCollection.collider.center.z + meshCollection.collider.extent.z;

				std::cout << "WALL BOUNDS: " << collectionName
					<< " X: [" << minX << ", " << maxX << "]"
					<< " Y: [" << minY << ", " << maxY << "]"
					<< " Z: [" << minZ << ", " << maxZ << "]" << std::endl;

				std::cout << "Wall entity created from model: " << collectionName << std::endl;
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
		registry.emplace<GAME::MeshCollection>(gameEntity);

		// Add a Transform component with identity matrix initially
		auto& transform = registry.emplace<GAME::Transform>(gameEntity);
		GW::MATH::GMatrix::IdentityF(transform.matrix);

		// Get entities from the model collection
		auto modelEntities = GAME::GetEntitiesFromCollection(registry, modelName);
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
			auto& meshCollection = registry.get<GAME::MeshCollection>(gameEntity);
			meshCollection.meshEntities.push_back(meshEntity);
		}

		return gameEntity;
	}

	// Toggle visibility of an entity
	void ToggleEntityVisibility(entt::registry& registry, entt::entity entity) {
		// Get the mesh collection for this entity
		if (!registry.all_of<GAME::MeshCollection>(entity)) {
			return;
		}

		auto& meshCollection = registry.get<GAME::MeshCollection>(entity);

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
		if (!registry.all_of<GAME::MeshCollection>(entity)) {
			return;
		}

		auto& meshCollection = registry.get<GAME::MeshCollection>(entity);

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
		auto& gameManager = registry.ctx().get<GAME::GameManager>();

		// Check for P key press to toggle player visibility
		float pKey = 0.0f;
		static bool pKeyPressed = false;
		input.immediateInput.GetState(G_KEY_P, pKey);

		if (pKey > 0.0f && !pKeyPressed) {
			pKeyPressed = true;
			gameManager.playerVisible = !gameManager.playerVisible;

			// Find the player entity
			auto playerView = registry.view<GAME::Player>();
			if (playerView.begin() != playerView.end()) {
				auto playerEntity = *playerView.begin();
				GAME::SetEntityVisibility(registry, playerEntity, gameManager.playerVisible);
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
			auto enemyView = registry.view<GAME::Enemy>();
			if (enemyView.begin() != enemyView.end()) {
				auto enemyEntity = *enemyView.begin();
				GAME::SetEntityVisibility(registry, enemyEntity, gameManager.enemyVisible);
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

	// Check if two entities are colliding
	bool AreEntitiesColliding(entt::registry& registry, entt::entity entity1, entt::entity entity2) {
		// Get the transforms and mesh collections for both entities
		auto& transform1 = registry.get<Transform>(entity1);
		auto& meshCollection1 = registry.get<MeshCollection>(entity1);
		auto& transform2 = registry.get<Transform>(entity2);
		auto& meshCollection2 = registry.get<MeshCollection>(entity2);

		// Get the positions from the transforms
		GW::MATH::GVECTORF pos1, pos2;
		GW::MATH::GMatrix::GetTranslationF(transform1.matrix, pos1);
		GW::MATH::GMatrix::GetTranslationF(transform2.matrix, pos2);

		// Check if either entity is a wall
		bool isWall1 = registry.all_of<Wall>(entity1);
		bool isWall2 = registry.all_of<Wall>(entity2);

		// If we're dealing with a wall, use the wall's collider
		if (isWall1 || isWall2) {
			// Determine which entity is the wall and which is the other entity
			auto wallEntity = isWall1 ? entity1 : entity2;
			auto otherEntity = isWall1 ? entity2 : entity1;
			auto& wallTransform = isWall1 ? transform1 : transform2;
			auto& wallMeshCollection = isWall1 ? meshCollection1 : meshCollection2;
			auto& otherPos = isWall1 ? pos2 : pos1;

			// Get wall position
			GW::MATH::GVECTORF wallPos;
			GW::MATH::GMatrix::GetTranslationF(wallTransform.matrix, wallPos);

			// Get the wall's collider center and transform it to world space
			GW::MATH::GVECTORF colliderCenter = wallMeshCollection.collider.center;
			GW::MATH::GVECTORF worldColliderCenter = {
				wallPos.x + colliderCenter.x,
				wallPos.y + colliderCenter.y,
				wallPos.z + colliderCenter.z
			};

			// Use the extents directly without additional scaling
			// The extents are already set appropriately in CreateWalls
			float wallExtentX = wallMeshCollection.collider.extent.x;
			float wallExtentY = wallMeshCollection.collider.extent.y;
			float wallExtentZ = wallMeshCollection.collider.extent.z;

			// Calculate distance from other entity to world collider center
			float dx = otherPos.x - worldColliderCenter.x;
			float dy = otherPos.y - worldColliderCenter.y;
			float dz = otherPos.z - worldColliderCenter.z;

			// Debug output for collision check
			std::cout << "Collision check: Entity at (" << otherPos.x << ", " << otherPos.y << ", " << otherPos.z
				<< ") with wall center at (" << worldColliderCenter.x << ", " << worldColliderCenter.y << ", " << worldColliderCenter.z
				<< "), distances: (" << dx << ", " << dy << ", " << dz
				<< "), extents: (" << wallExtentX << ", " << wallExtentY << ", " << wallExtentZ << ")" << std::endl;

			// Add a small collision radius for non-wall entities (1.0 unit)
			float collisionRadius = 1.0f;

			// Check if the other entity is within the wall's boundaries
			bool withinX = std::abs(dx) < wallExtentX + collisionRadius;
			bool withinY = std::abs(dy) < wallExtentY + collisionRadius;
			bool withinZ = std::abs(dz) < wallExtentZ + collisionRadius;

			// Debug output for collision result
			if (withinX && withinY && withinZ) {
				std::cout << "COLLISION DETECTED with wall!" << std::endl;
			}

			return withinX && withinY && withinZ;
		}
		else {
			// For non-wall collisions, use a simple distance-based approach
			float dx = pos2.x - pos1.x;
			float dy = pos2.y - pos1.y;
			float dz = pos2.z - pos1.z;
			float distanceSquared = dx * dx + dy * dy + dz * dz;

			// Check if distance is less than twice the collision radius
			// Using a collision radius of 1.0 for all non-wall entities
			float collisionRadius = 1.0f;
			float collisionDistanceSquared = collisionRadius * collisionRadius * 4.0f; // (radius1 + radius2)^2

			return distanceSquared < collisionDistanceSquared;
		}
	}

	// Handle collision between two entities
	void HandleCollision(entt::registry& registry, entt::entity entity1, entt::entity entity2) {
		// Check entity types and handle collisions accordingly
		bool isPlayer1 = registry.all_of<Player>(entity1);
		bool isPlayer2 = registry.all_of<Player>(entity2);
		bool isEnemy1 = registry.all_of<Enemy>(entity1);
		bool isEnemy2 = registry.all_of<Enemy>(entity2);
		bool isBullet1 = registry.all_of<Bullet>(entity1);
		bool isBullet2 = registry.all_of<Bullet>(entity2);
		bool isWall1 = registry.all_of<Wall>(entity1);
		bool isWall2 = registry.all_of<Wall>(entity2);

		// Player-Wall collision: prevent player from moving through walls
		if ((isPlayer1 && isWall2) || (isPlayer2 && isWall1)) {
			auto playerEntity = isPlayer1 ? entity1 : entity2;
			auto wallEntity = isPlayer1 ? entity2 : entity1;

			// Get the player's transform
			auto& playerTransform = registry.get<Transform>(playerEntity);
			auto& wallTransform = registry.get<Transform>(wallEntity);
			auto& wallMeshCollection = registry.get<MeshCollection>(wallEntity);

			// Get positions
			GW::MATH::GVECTORF playerPos, wallPos;
			GW::MATH::GMatrix::GetTranslationF(playerTransform.matrix, playerPos);
			GW::MATH::GMatrix::GetTranslationF(wallTransform.matrix, wallPos);

			// Get the wall's collider center and transform it to world space
			GW::MATH::GVECTORF colliderCenter = wallMeshCollection.collider.center;
			GW::MATH::GVECTORF worldColliderCenter = {
				wallPos.x + colliderCenter.x,
				wallPos.y + colliderCenter.y,
				wallPos.z + colliderCenter.z
			};

			// Calculate direction from wall center to player
			GW::MATH::GVECTORF direction = {
				playerPos.x - worldColliderCenter.x,
				0.0f, // No vertical component to avoid pushing player under floor
				playerPos.z - worldColliderCenter.z
			};

			// Normalize the direction vector (avoid division by zero)
			float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
			if (length > 0.001f) {
				direction.x /= length;
				direction.z /= length;
			}
			else {
				// If we're directly above/below the wall, determine which side to push based on player position
				if (playerPos.x > worldColliderCenter.x) {
					direction.x = 1.0f;
				}
				else {
					direction.x = -1.0f;
				}
			}

			// Move the player away from the wall with a stronger push
			GW::MATH::GVECTORF pushOut = {
				direction.x * 1.0f, // Stronger push
				0.0f, // No vertical push
				direction.z * 1.0f  // Stronger push
			};

			GW::MATH::GMatrix::TranslateGlobalF(playerTransform.matrix, pushOut, playerTransform.matrix);

			std::cout << "Player collided with wall - pushed out with vector: ("
				<< pushOut.x << ", " << pushOut.y << ", " << pushOut.z << ")" << std::endl;
		}

		// Enemy-Wall collision: make enemy bounce off wall
	// Enemy-Wall collision: make enemy bounce off wall
		if ((isEnemy1 && isWall2) || (isEnemy2 && isWall1)) {
			auto enemyEntity = isEnemy1 ? entity1 : entity2;
			auto wallEntity = isEnemy1 ? entity2 : entity1;

			// Get the enemy's transform and velocity
			auto& enemyTransform = registry.get<Transform>(enemyEntity);

			// Check if the enemy has a velocity component
			if (registry.all_of<Velocity>(enemyEntity)) {
				auto& enemyVelocity = registry.get<Velocity>(enemyEntity);
				auto& wallTransform = registry.get<Transform>(wallEntity);
				auto& wallMeshCollection = registry.get<MeshCollection>(wallEntity);

				// Get positions
				GW::MATH::GVECTORF enemyPos, wallPos;
				GW::MATH::GMatrix::GetTranslationF(enemyTransform.matrix, enemyPos);
				GW::MATH::GMatrix::GetTranslationF(wallTransform.matrix, wallPos);

				// Get the wall's collider center and transform it to world space
				GW::MATH::GVECTORF colliderCenter = wallMeshCollection.collider.center;
				GW::MATH::GVECTORF worldColliderCenter = {
					wallPos.x + colliderCenter.x,
					wallPos.y + colliderCenter.y,
					wallPos.z + colliderCenter.z
				};

				// Calculate direction from wall center to enemy
				GW::MATH::GVECTORF direction = {
					enemyPos.x - worldColliderCenter.x,
					0.0f, // No vertical component
					enemyPos.z - worldColliderCenter.z
				};

				// Determine which axis to reflect based on wall position
				bool reflectX = false;
				bool reflectZ = false;

				// Check if it's a left/right wall or top/bottom wall
				if (wallPos.x < -15.0f || wallPos.x > 15.0f) {
					// Left or right wall - reflect X component
					reflectX = true;
				}
				else if (wallPos.z < -15.0f || wallPos.z > 15.0f) {
					// Top or bottom wall - reflect Z component
					reflectZ = true;
				}

				// Reflect the velocity vector
				if (reflectX) {
					enemyVelocity.direction.x = -enemyVelocity.direction.x;
				}
				if (reflectZ) {
					enemyVelocity.direction.z = -enemyVelocity.direction.z;
				}

				// Move the enemy away from the wall slightly to prevent sticking
				GW::MATH::GVECTORF pushOut = {
					enemyVelocity.direction.x * 0.5f,
					0.0f,
					enemyVelocity.direction.z * 0.5f
				};

				GW::MATH::GMatrix::TranslateGlobalF(enemyTransform.matrix, pushOut, enemyTransform.matrix);

				std::cout << "Enemy bounced off wall with new direction: ("
					<< enemyVelocity.direction.x << ", "
					<< enemyVelocity.direction.y << ", "
					<< enemyVelocity.direction.z << ")" << std::endl;
			}
			else {
				// Fallback for enemies without velocity - simple push
				GW::MATH::GVECTORF moveBack = { -0.1f, 0.0f, -0.1f };
				GW::MATH::GMatrix::TranslateGlobalF(enemyTransform.matrix, moveBack, enemyTransform.matrix);
				std::cout << "Enemy without velocity collided with wall" << std::endl;
			}
		}

		// Bullet-Wall collision: destroy bullet
		if ((isBullet1 && isWall2) || (isBullet2 && isWall1)) {
			auto bulletEntity = isBullet1 ? entity1 : entity2;

			// Destroy the bullet
			registry.destroy(bulletEntity);

			std::cout << "Bullet collided with wall and was destroyed" << std::endl;
		}

		// Bullet-Enemy collision: destroy both
		if ((isBullet1 && isEnemy2) || (isBullet2 && isEnemy1)) {
			auto bulletEntity = isBullet1 ? entity1 : entity2;
			auto enemyEntity = isEnemy1 ? entity1 : entity2;

			// Destroy both entities
			registry.destroy(bulletEntity);
			registry.destroy(enemyEntity);

			std::cout << "Bullet hit enemy! Both were destroyed" << std::endl;
		}
	}

	void GAME::AdjustWallCollider(entt::registry& registry, entt::entity wallEntity, const GW::MATH::GMATRIXF& transform, const std::string& wallName)
	{
		// Get the wall's position
		GW::MATH::GVECTORF wallPos;
		GW::MATH::GMatrix::GetTranslationF(transform, wallPos);

		// Get the wall's mesh collection
		auto& meshCollection = registry.get<MeshCollection>(wallEntity);

		// Determine which wall this is based on position
		// Left wall (negative X)
		if (wallPos.x < -15.0f) {
			std::cout << "ADJUSTING LEFT WALL COLLIDER" << std::endl;
			meshCollection.collider.center = { 0.0f, 0.0f, 0.0f };
			meshCollection.collider.extent = { 1.0f, 10.0f, 20.0f };
		}
		// Right wall (positive X)
		else if (wallPos.x > 15.0f) {
			std::cout << "ADJUSTING RIGHT WALL COLLIDER" << std::endl;
			meshCollection.collider.center = { 0.0f, 0.0f, 0.0f };
			meshCollection.collider.extent = { 1.0f, 10.0f, 20.0f };
		}
		// Top wall (positive Z)
		else if (wallPos.z > 15.0f) {
			std::cout << "ADJUSTING TOP WALL COLLIDER" << std::endl;
			meshCollection.collider.center = { 0.0f, 0.0f, 0.0f };
			meshCollection.collider.extent = { 20.0f, 10.0f, 1.0f };
		}
		// Bottom wall (negative Z)
		else if (wallPos.z < -15.0f) {
			std::cout << "ADJUSTING BOTTOM WALL COLLIDER" << std::endl;
			meshCollection.collider.center = { 0.0f, 0.0f, 0.0f };
			meshCollection.collider.extent = { 20.0f, 10.0f, 1.0f };
		}
	}


	// Check for collisions between collidable entities
	void CheckCollisions(entt::registry& registry) {
		// Get all entities with Transform, MeshCollection, and Collidable components
		auto collidableView = registry.view<Transform, MeshCollection, Collidable>();
		
		// Collect collision pairs to process
		std::vector<std::pair<entt::entity, entt::entity>> collisionPairs;
		std::set<std::pair<entt::entity, entt::entity>> processedPairs;

		// For each collidable entity
		for (auto entity1 : collidableView) {
			// For each other collidable entity
			for (auto entity2 : collidableView) {
				// Skip self-collision
				if (entity1 == entity2) continue;
				
				// Skip if we have already processed this pair (avoid duplicate collision handling)
				auto pair1 = std::make_pair(entity1, entity2);
				auto pair2 = std::make_pair(entity2, entity1);
				if (processedPairs.find(pair1) != processedPairs.end() || 
					processedPairs.find(pair2) != processedPairs.end()) {
					continue;
				}

				// Check if the entities are colliding
				if (GAME::AreEntitiesColliding(registry, entity1, entity2)) {
					// Add to collision pairs and mark as processed
					collisionPairs.push_back(pair1);
					processedPairs.insert(pair1);
				}
			}
		}
		
		// Process all collisions after detection is complete
		for (const auto& pair : collisionPairs) {
			// Verify entities still exist before handling collision
			if (registry.valid(pair.first) && registry.valid(pair.second)) {
				GAME::HandleCollision(registry, pair.first, pair.second);
			}
		}
	}

	void HandleEnemyShattering(entt::registry& registry, entt::entity enemyEntity) {
		// TODO: Implement enemy shattering logic
		// For now, just destroy the enemy
		registry.destroy(enemyEntity);
		std::cout << "Enemy shattered!" << std::endl;
	}
}// namespace GAME
