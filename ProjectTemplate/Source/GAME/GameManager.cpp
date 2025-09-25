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
		std::cout << "BounceEnemy called for entity at position: (" << enemyLocation.x << ", " << enemyLocation.z << ")" << std::endl;
		std::cout << "Enemy velocity: (" << enemyVelocity.x << ", " << enemyVelocity.z << ")" << std::endl;

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
		//std::cout << "\=== DETAILED COLLISION DEBUG ===" << std::endl;
		//std::cout << "Total entities in collision view: " << collisions.size_hint() << std::endl;

		//// Print detailed entity information
		//for (auto entity : collisions) {
		//	std::cout << "Entity " << (int)entity << ": ";
		//	if (registry.all_of<GAME::Enemy>(entity)) std::cout << "Enemy ";
		//	if (registry.all_of<GAME::Obstacle>(entity)) std::cout << "Obstacle ";
		//	if (registry.all_of<GAME::Bullet>(entity)) std::cout << "Bullet ";
		//	if (registry.all_of<GAME::Player>(entity)) std::cout << "Player ";

		//	auto& transform = registry.get<GAME::Transform>(entity);
		//	auto& meshCollection = registry.get<GAME::MeshCollection>(entity);

		//	std::cout << std::endl;
		//	std::cout << "  Position: ("
		//		<< transform.matrix.row4.x << ", "
		//		<< transform.matrix.row4.y << ", "
		//		<< transform.matrix.row4.z << ")" << std::endl;

		//	std::cout << "  Collider: Center=("
		//		<< meshCollection.collider.center.x << ", "
		//		<< meshCollection.collider.center.y << ", "
		//		<< meshCollection.collider.center.z << "), ";

		//	std::cout << "Extent=("
		//		<< meshCollection.collider.extent.x << ", "
		//		<< meshCollection.collider.extent.y << ", "
		//		<< meshCollection.collider.extent.z << ")" << std::endl;
		//}
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
			colA.center = transA.row4;

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
				colB.center = transB.row4;

				//Rotate
				GQUATERNIONF qB;
				GQuaternion::SetByMatrixF(transB, qB);
				GQuaternion::MultiplyQuaternionF(colB.rotation, qB, colB.rotation);

				GCollision::GCollisionCheck result;
				GCollision::TestOBBToOBBF(colA, colB, result);
				if (GCollision::GCollisionCheck::COLLISION == result)
				{
					if (registry.all_of<Obstacle>(*a) && registry.all_of<Obstacle>(*b)) {
						continue; // Skip to next iteration
					}
					// These 2 are colliding!
					std::cout << "COLLISION DETECTED between entities " << (int)*a << " and " << (int)*b << std::endl;
					std::cout << "  Entity A: Enemy=" << registry.all_of<Enemy>(*a) << ", Obstacle=" << registry.all_of<Obstacle>(*a) << ", Player=" << registry.all_of<Player>(*a) << std::endl;
					std::cout << "  Entity B: Enemy=" << registry.all_of<Enemy>(*b) << ", Obstacle=" << registry.all_of<Obstacle>(*b) << ", Player=" << registry.all_of<Player>(*b) << std::endl;
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
					if (registry.all_of<Bullet>(*a) && registry.all_of<Enemy>(*b))
					{
						// Destroy the bullet
						registry.emplace_or_replace<toDestroy>(*a);
						// Reduce enemy health
						auto& health = registry.get<Health>(*b);
						health.current--;
						std::cout << "Enemy hit! Health reduced to: " << health.current << std::endl;
					}
					if (registry.all_of<Bullet>(*b) && registry.all_of<Enemy>(*a))
					{
						// Destroy the bullet
						registry.emplace_or_replace<toDestroy>(*b);
						// Reduce enemy health
						auto& health = registry.get<Health>(*a);
						health.current--;
						std::cout << "Enemy hit! Health reduced to: " << health.current << std::endl;
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
			auto enemyHealthView = registry.view<Enemy, Health>();
			for (auto enemyEntity : enemyHealthView)
			{
				auto& health = registry.get<Health>(enemyEntity);
				if (health.current <= 0)
				{
					// Check if this enemy shatters
					if (registry.all_of<Shatters>(enemyEntity))
					{
						auto& shatters = registry.get<Shatters>(enemyEntity);
						auto& transform = registry.get<Transform>(enemyEntity);

						// Get shatter configuration from config file
						std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
						int shatterAmount = 2; // Default value
						float shatterScale = 0.7f; // Default value
						try {
							shatterAmount = config->at("Enemy1").at("shatterAmount").as<int>();
							shatterScale = config->at("Enemy1").at("shatterScale").as<float>();
						}
						catch (const std::exception& e) {
							std::cout << "Shatter config not found, using defaults: " << e.what() << std::endl;
						}

						// Set up velocity for shattered enemies
						GW::MATH::GVECTORF controlledDirection = { -1.0f, 0.0f, 0.0f }; // Moving left toward wall
						if (rand() % 2 == 0) controlledDirection.x = 1.0f; // Random direction
						if (rand() % 2 == 0) controlledDirection.z = (float)(rand() % 10 - 5) * 0.1f; // Random Z component

						// Create shatterAmount new enemies
						for (int i = 0; i < shatterAmount; i++)
						{
							// Create new enemy entity
							entt::entity newEnemy = registry.create();

							// Copy basic components from original enemy
							registry.emplace<GAME::Transform>(newEnemy);
							registry.emplace<GAME::MeshCollection>(newEnemy);
							registry.emplace<GAME::Collidable>(newEnemy);
							registry.emplace<GAME::Health>(newEnemy, health.maximum, health.maximum);
							registry.emplace<GAME::Velocity>(newEnemy, controlledDirection, 3.0f);

							// Set up transform with scaled position near original
							auto& newTransform = registry.get<GAME::Transform>(newEnemy);
							newTransform.matrix = transform.matrix;
							newTransform.matrix.row4.x += (float)(rand() % 10 - 5) * 0.1f; // Small random offset
							newTransform.matrix.row4.z += (float)(rand() % 10 - 5) * 0.1f; // Small random offset

							// Apply scale reduction
							GW::MATH::GVECTORF scale = { shatterScale, shatterScale, shatterScale };
							GW::MATH::GMatrix::ScaleGlobalF(newTransform.matrix, scale, newTransform.matrix);

							// Decrement shatter count and only add Shatters if more remain
							int newShatterCount = shatters.remaining - 1;
							if (newShatterCount > 0)
							{
								registry.emplace<GAME::Shatters>(newEnemy, newShatterCount);
							}

							// Add Enemy tag
							registry.emplace<GAME::Enemy>(newEnemy);

							std::cout << "Created shatter enemy " << i + 1 << " with scale " << shatterScale << std::endl;
						}

						// Mark original enemy for destruction
						registry.emplace_or_replace<toDestroy>(enemyEntity);
						std::cout << "Enemy shattered into " << shatterAmount << " smaller enemies!" << std::endl;
					}
					else
					{
						// No shatters remaining, just destroy the enemy
						registry.emplace_or_replace<toDestroy>(enemyEntity);
						std::cout << "Enemy destroyed (no shatters remaining)!" << std::endl;
					}
				}
				else
				{
					// Health > 0, enemy is still alive
					// No action needed here, enemy continues to exist
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