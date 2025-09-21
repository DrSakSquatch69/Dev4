#include "GameManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"
#include <vector>
#include <set>
#include <algorithm>
using namespace GW::MATH;
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
		auto& collisions = registry.view<Transform, MeshCollection, Collidable>();
		for (auto a = collisions.begin(); a != collisions.end(); a++)
		{
			auto colA = registry.get<MeshCollection>(*a).collider;
			auto& transA = registry.get<Transform>(*a).matrix;

			// Scale the extents
			GVECTORF vecA;
			GMatrix::GetScaleF(transA, vecA);
			colA.extent.x = vecA.x;
			colA.extent.y = vecA.y;
			colA.extent.z = vecA.z;

			//Transform the center
			GMatrix::VectorXMatrixF(transA, colA.center, colA.center);

			//Rotate
			GQUATERNIONF qA;
			GQuaternion::SetByMatrixF(transA, qA);
			GQuaternion::MultiplyQuaternionF(colA.rotation, qA, colA.rotation);
			 
			auto b = a;
			for (b++; b != collisions.end(); b++)
			{
				auto colB = registry.get<MeshCollection>(*b).collider;
				auto& transB = registry.get<Transform>(*b).matrix;

				// Scale the extents
				GVECTORF vecB;
				GMatrix::GetScaleF(transB, vecB);
				colB.extent.x = vecB.x;
				colB.extent.y = vecB.y;
				colB.extent.z = vecB.z;

				//Transform the center
				GMatrix::VectorXMatrixF(transB, colB.center, colB.center);

				//Rotate
				GQUATERNIONF qB;
				GQuaternion::SetByMatrixF(transB, qB);
				GQuaternion::MultiplyQuaternionF(colB.rotation, qB, colB.rotation);
			
				GCollision::GCollisionCheck result;
				GCollision::TestOBBToOBBF(colA, colB, result);
				if (GCollision::GCollisionCheck::COLLISION == result)
				{
					// These 2 are colliding!

					//bullet to wall
					if (registry.all_of<Bullet>(*a) && registry.all_of<Wall>(*b))
					{
						registry.emplace_or_replace<ToDestroy>(*a);
					}
					if (registry.all_of<Bullet>(*b) && registry.all_of<Wall>(*a))
					{
						registry.emplace_or_replace<ToDestroy>(*b);
					}
				}
			}
			auto& toDestroy = registry.view<ToDestroy>();
			for (auto ent : toDestroy)
			{
				registry.destroy(ent);
			}
		}
		
		// Update GPU instances from Transform components 
		UpdateGPUInstances(registry);
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
						0.5f,                // Half thickness (X) - thin but effective
						15.0f,               // Half height (Y) - much taller - assuming the wall is about 10 units tall
						25.0f                // Half length (Z) - much longer - assuming the wall is about 20 units long
					};
				}
				else if (wallPos.x > 15.0f) {
					// Right wall (positive X)
					std::cout << "CREATING RIGHT WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						0.5f,                // Half thickness (X) - thin but effective
						15.0f,               // Half height (Y) - much taller
						25.0f                // Half length (Z) - much longer
					};
				}
				else if (wallPos.z > 15.0f) {
					// Top wall (positive Z)
					std::cout << "CREATING TOP WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						25.0f,               // Half width (X) - much wider
						15.0f,               // Half height (Y) - much taller
						0.5f                 // Half thickness (Z) - thin but effective
					};
				}
				else if (wallPos.z < -15.0f) {
					// Bottom wall (negative Z)
					std::cout << "CREATING BOTTOM WALL COLLIDER" << std::endl;
					meshCollection.collider.extent = {
						25.0f,               // Half width (X) - much wider
						15.0f,               // Half height (Y) - much taller
						0.5f                 // Half thickness (Z) - thin but effective
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

}// namespace GAME
