#include "GameManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"

namespace GAME {

	void InitializeGameManager(entt::registry& registry) {
		// Create a GameManager in the registry context
		registry.ctx().emplace<GameManager>();
		std::cout << "GameManager initialized" << std::endl;
	}

	// Pseudocode plan:
	// 1. The error is caused by calling registry.view() with no component types, which is not valid in EnTT v3+.
	// 2. To iterate over all entities, use registry.each() instead of registry.view().
	// 3. If you want to iterate over entities with a specific component (e.g., Player), use registry.view<Player>().
	// 4. Fix the line in UpdateGameManager that currently reads: auto playerView = registry.view();
	// 5. Replace it with registry.view<Player>() if you want all Player entities, or use registry.each() for all entities.

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

				// Set the wall's collider from the level data
				auto& meshCollection = registry.get<MeshCollection>(wallEntity);
				if (model.colliderIndex < cpuLevel.lvlData.levelColliders.size()) {
					// Get the collider from the level data
					meshCollection.collider = cpuLevel.lvlData.levelColliders[model.colliderIndex];

					// Override colliders with manual settings based on wall position
					AdjustWallCollider(registry, wallEntity, transform.matrix, collectionName);

					// Get wall position from transform
					GW::MATH::GVECTORF wallPos;
					GW::MATH::GMatrix::GetTranslationF(transform.matrix, wallPos);

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
					GW::MATH::GVECTORF scale;
					GW::MATH::GMatrix::GetScaleF(transform.matrix, scale);
					std::cout << "WALL TRANSFORM: " << collectionName
						<< " scale: (" << scale.x << ", " << scale.y << ", " << scale.z << ")" << std::endl;

					// Print the full transform matrix for debugging
					std::cout << "WALL MATRIX: " << collectionName << std::endl;
					for (int i = 0; i < 4; i++) {
						std::cout << "  [";
						for (int j = 0; j < 4; j++) {
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
						}
				else {
					std::cout << "Warning: No collider found for wall model at index " << model.colliderIndex << std::endl;
				}

				std::cout << "Wall entity created from model: " << collectionName << std::endl;
					}
				}
			}
		}

		// Map to store collections of entities by name
		std::map<std::string, std::vector<entt::entity>> modelCollections;

		void AddEntityToCollection(entt::registry & registry, entt::entity entity, const std::string & collectionName) {
			modelCollections[collectionName].push_back(entity);
		}

		std::vector<entt::entity> GetEntitiesFromCollection(entt::registry & registry, const std::string & collectionName) {
			if (modelCollections.find(collectionName) != modelCollections.end()) {
				return modelCollections[collectionName];
			}
			return std::vector<entt::entity>();
		}

		entt::entity CreateGameEntityFromModel(entt::registry & registry, const std::string & modelName) {
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
		void ToggleEntityVisibility(entt::registry & registry, entt::entity entity) {
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
		void SetEntityVisibility(entt::registry & registry, entt::entity entity, bool visible) {
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
		void HandleVisibilityToggleInput(entt::registry & registry) {
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
		void on_update(entt::registry & registry, entt::entity entity) {
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

		// Check for collisions between collidable entities
		void CheckCollisions(entt::registry & registry) {
			// Get all entities with Transform, MeshCollection, and Collidable components
			auto collidableView = registry.view<Transform, MeshCollection, Collidable>();

			// For each collidable entity
			for (auto entity1 : collidableView) {
				// For each other collidable entity
				for (auto entity2 : collidableView) {
					// Skip self-collision
					if (entity1 == entity2) continue;

					// Check if the entities are colliding
					if (AreEntitiesColliding(registry, entity1, entity2)) {
						// Handle the collision
						HandleCollision(registry, entity1, entity2);
					}
				}
			}
		}

		// Check if two entities are colliding
		bool AreEntitiesColliding(entt::registry & registry, entt::entity entity1, entt::entity entity2) {
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

				// Get the wall's extents and scale them by the wall's transform
				GW::MATH::GVECTORF wallScale;
				GW::MATH::GMatrix::GetScaleF(wallTransform.matrix, wallScale);

				// Apply a multiplier to make the colliders larger
				const float COLLIDER_SCALE_MULTIPLIER = 2.0f;

				float wallExtentX = wallMeshCollection.collider.extent.x * wallScale.x * COLLIDER_SCALE_MULTIPLIER;
				float wallExtentY = wallMeshCollection.collider.extent.y * wallScale.y * COLLIDER_SCALE_MULTIPLIER;
				float wallExtentZ = wallMeshCollection.collider.extent.z * wallScale.z * COLLIDER_SCALE_MULTIPLIER;

				// Make sure extents are not too small
				wallExtentX = std::max<float>(wallExtentX, 2.0f);
				wallExtentY = std::max<float>(wallExtentY, 2.0f);
				wallExtentZ = std::max<float>(wallExtentZ, 2.0f);

				// Calculate distance from other entity to world collider center
				float dx = otherPos.x - worldColliderCenter.x;
				float dy = otherPos.y - worldColliderCenter.y;
				float dz = otherPos.z - worldColliderCenter.z;

				// Debug output for collision check
				std::cout << "Collision check: Entity at (" << otherPos.x << ", " << otherPos.y << ", " << otherPos.z
					<< ") with wall center at (" << worldColliderCenter.x << ", " << worldColliderCenter.y << ", " << worldColliderCenter.z
					<< "), distances: (" << dx << ", " << dy << ", " << dz
					<< "), extents: (" << wallExtentX << ", " << wallExtentY << ", " << wallExtentZ << ")" << std::endl;

				// Check if the other entity is within the wall's boundaries
				bool withinX = std::abs(dx) < wallExtentX + 1.0f; // Add 1.0f for collision radius
				bool withinY = std::abs(dy) < wallExtentY + 1.0f;
				bool withinZ = std::abs(dz) < wallExtentZ + 1.0f;

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
				float collisionDistanceSquared = 4.0f; // 2.0f radius squared

				return distanceSquared < collisionDistanceSquared;
			}
		}

		// Handle collision between two entities
		void HandleCollision(entt::registry & registry, entt::entity entity1, entt::entity entity2) {
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
			if ((isEnemy1 && isWall2) || (isEnemy2 && isWall1)) {
				auto enemyEntity = isEnemy1 ? entity1 : entity2;

				// Get the enemy's transform
				auto& enemyTransform = registry.get<Transform>(enemyEntity);

				// Simple bounce: move the enemy back slightly
				GW::MATH::GVECTORF moveBack = { -0.1f, 0.0f, -0.1f };
				GW::MATH::GMatrix::TranslateGlobalF(enemyTransform.matrix, moveBack, enemyTransform.matrix);

				std::cout << "Enemy collided with wall" << std::endl;
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

		void GAME::AdjustWallCollider(entt::registry & registry, entt::entity wallEntity, const GW::MATH::GMATRIXF & transform, const std::string & wallName) {
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
	} // namespace GAME// Adjust wall collider based on position