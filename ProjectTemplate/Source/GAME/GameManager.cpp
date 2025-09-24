#include "GameManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"
using namespace GW::MATH;

namespace GAME {

	void InitializeGameManager(entt::registry& registry) {
		// Create a GameManager in the registry context
		registry.ctx().emplace<GameManager>();
		std::cout << "GameManager initialized" << std::endl;
	}

	void BounceEnemy(GVECTORF enemyLocation, GVECTORF& enemyVelocity, GOBBF& obstacleBox)
	{
		//Find Normal
		GVECTORF normal;
		GCollision::ClosestPointToOBBF(obstacleBox, enemyLocation, normal);
		GVector::SubtractVectorF(enemyLocation, normal, normal);
		normal.y = 0.0f; normal.w = 0.0f;
		GVector::NormalizeF(normal, normal);

		//w = v - (2 * (v * n) * n)
		float dot;
		GVector::DotF(enemyVelocity, normal, dot);
		dot *= 2.0f;
		GVector::ScaleF(normal, dot, normal);
		GVector::SubtractVectorF(enemyVelocity, normal, enemyVelocity);

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
		
		UpdateVelocitySystem(registry, deltaTime);

		// Collision system
		// Check for collisions between entities
		auto& collisions = registry.view<Transform, MeshCollection, Collidable>();
		for (auto a = collisions.begin(); a != collisions.end(); a++)
		{
			auto colA = registry.get<MeshCollection>(*a).collider;
			auto& transA = registry.get<Transform>(*a).matrix;

			// Scale the extents
			GVECTORF vecA;
			GMatrix::GetScaleF(transA, vecA);
			colA.extent.x *= vecA.x;
			colA.extent.y *= vecA.y;
			colA.extent.z *= vecA.z;

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
				colB.extent.x *= vecB.x;
				colB.extent.y *= vecB.y;
				colB.extent.z *= vecB.z;

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
					if (registry.all_of<Bullet>(*a) && registry.all_of<Obstacle>(*b))
					{
						registry.emplace_or_replace<toDestroy>(*a);
					}
					if (registry.all_of<Bullet>(*b) && registry.all_of<Obstacle>(*a))
					{
						registry.emplace_or_replace<toDestroy>(*b);
					}
					// Enemy to wall - bounce response
					if (registry.all_of<Enemy>(*a) && registry.all_of<Obstacle>(*b))
					{
						auto& vel = registry.get<Velocity>(*a).direction;
						BounceEnemy(transA.row4, vel, colB);
					}
					if (registry.all_of<Enemy>(*b) && registry.all_of<Obstacle>(*a))
					{
						auto& vel = registry.get<Velocity>(*b).direction;
						BounceEnemy(transB.row4, vel, colA);
					}
				}
			}
			auto& ToDestroy = registry.view<toDestroy>();
			if (ToDestroy.size() > 0) {
				std::cout << "Destroying " << ToDestroy.size() << " entities this frame" << std::endl; 
			}
			for (auto ent : ToDestroy)
			{
				// If this entity has a MeshCollection, destroy all its mesh entities too
				if (registry.all_of<MeshCollection>(ent)) {
					auto& meshCollection = registry.get<MeshCollection>(ent);
					std::cout << "  - Entity has " << meshCollection.meshEntities.size() << " mesh entities" << std::endl;

					for (auto meshEntity : meshCollection.meshEntities) {
						std::cout << "  - Destroying mesh entity: " << (int)meshEntity << std::endl;
						registry.destroy(meshEntity);
					}
				}

				registry.destroy(ent);
				std::cout << "  - Main entity destroyed successfully" << std::endl;
			}
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

	void UpdateVelocitySystem(entt::registry& registry, float deltaTime) {
		// Get all entities with both Transform and Velocity
		auto view = registry.view<Transform, Velocity>();

		for (auto entity : view) {
			auto& transform = registry.get<Transform>(entity);
			auto& velocity = registry.get<Velocity>(entity);

			// Calculate movement for this frame
			GW::MATH::GVECTORF movement = {
				velocity.direction.x * velocity.speed * deltaTime,
				velocity.direction.y * velocity.speed * deltaTime,
				velocity.direction.z * velocity.speed * deltaTime
			};

			// Apply the movement
			GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
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