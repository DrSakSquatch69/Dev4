// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "APP/Window.hpp"
#include "GAME/GameManager.h"
#include "UTIL/GameConfig.h"
#include "GAME/Player.h"
#include "GAME/CollisionSystem.h"
#include "GAME/CollisionHelper.h"

// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);
void CreatePlayer(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);
	GAME::InitializeModelManager(registry);
	GAME::InitializeCollisionSystem(registry);

	// Seed the rand
	unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
	srand(time);

	registry.ctx().emplace<UTIL::Config>();

	GraphicsBehavior(registry); // create windows, surfaces, and renderers

	GameplayBehavior(registry); // create entities and components for gameplay

	MainLoopBehavior(registry); // update windows and input


	// clear all entities and components from the registry
	// invokes on_destroy() for all components that have it
	// registry will still be intact while this is happening
	registry.clear();

	return 0; // now destructors will be called for all components
}

void CreatePlayer(entt::registry& registry)
{
	// Create a player entity from the Turtle model
	entt::entity playerEntity = GAME::CreateGameEntityFromModel(registry, "Turtle");

	// Check if the entity has a MeshCollection component
	if (registry.all_of<GAME::MeshCollection>(playerEntity) &&
		!registry.get<GAME::MeshCollection>(playerEntity).meshEntities.empty()) {
		// Add the Player tag to the entity
		registry.emplace<GAME::Player>(playerEntity);

		// Position the player at a suitable starting position
		auto& transform = registry.get<GAME::Transform>(playerEntity);
		GW::MATH::GVECTORF startPosition = { 0.0f, 0.0f, 0.0f };
		GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, startPosition, transform.matrix);

		// Make sure the MeshCollection has a properly initialized collider
		auto& meshCollection = registry.get<GAME::MeshCollection>(playerEntity);
		// Initialize the collider with default values - INCREASED SIZE
		meshCollection.collider.center = { 0.0f, 0.0f, 0.0f, 1.0f };
		meshCollection.collider.extent = { 1.5f, 1.5f, 1.5f, 1.0f }; // Larger size for better collision
		meshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		std::cout << "Player collider initialized with size: " << meshCollection.collider.extent.x << std::endl;

		// Add the Collidable tag to the entity
		registry.emplace<GAME::Collidable>(playerEntity);
		std::cout << "Player entity created with Collidable tag" << std::endl;

		std::cout << "Player entity created successfully" << std::endl;
	}
	else {
		std::cout << "Failed to create player entity - model collection not found or empty" << std::endl;
	}
}

// This function will be called by the main loop to update the graphics
// It will be responsible for loading the Level, creating the VulkanRenderer, and all VulkanInstances
void GraphicsBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	// Add an entity to handle all the graphics data
	auto display = registry.create();

	auto LevelFile = (*config).at("Level1").at("levelFile").as<std::string>();
	auto ModelPath = (*config).at("Level1").at("modelPath").as<std::string>();

	// TODO: Emplace CPULevel. Placing here to reduce occurrence of a json race condition crash
	registry.emplace<DRAW::CPULevel>(display, DRAW::CPULevel{ LevelFile, ModelPath });

	CreatePlayer(registry);

	// Emplace and initialize Window component
	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();
	registry.emplace<APP::Window>(display,
		APP::Window{ startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, "Jacob Blackburn - Assignment 2" });


	// Create the input
	auto& input = registry.ctx().emplace<UTIL::Input>();
	auto& window = registry.get<GW::SYSTEM::GWindow>(display);
	input.bufferedInput.Create(window);
	input.immediateInput.Create(window);
	input.gamePads.Create();
	auto& pressEvents = registry.ctx().emplace<GW::CORE::GEventCache>();
	pressEvents.Create(32);
	input.bufferedInput.Register(pressEvents);
	input.gamePads.Register(pressEvents);

	// Create a transient component to initialize the Renderer
	std::string vertShader = (*config).at("Shaders").at("vertex").as<std::string>();
	std::string pixelShader = (*config).at("Shaders").at("pixel").as<std::string>();
	registry.emplace<DRAW::VulkanRendererInitialization>(display,
		DRAW::VulkanRendererInitialization{
			vertShader, pixelShader,
			{ {0.2f, 0.2f, 0.25f, 1} } , { 1.0f, 0u }, 75.f, 0.1f, 100.0f });
	registry.emplace<DRAW::VulkanRenderer>(display);

	// TODO : Emplace GPULevel
	registry.emplace<DRAW::GPULevel>(display);


	// Register for Vulkan clean up
	GW::CORE::GEventResponder shutdown;
	shutdown.Create([&](const GW::GEvent& e) {
		GW::GRAPHICS::GVulkanSurface::Events event;
		GW::GRAPHICS::GVulkanSurface::EVENT_DATA data;
		if (+e.Read(event, data) && event == GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES) {
			registry.clear<DRAW::VulkanRenderer>();
		}
		});
	registry.get<DRAW::VulkanRenderer>(display).vlkSurface.Register(shutdown);
	registry.emplace<GW::CORE::GEventResponder>(display, shutdown.Relinquish());

	// Create a camera and emplace it
	GW::MATH::GMATRIXF initialCamera;
	GW::MATH::GVECTORF translate = { 0.0f,  45.0f, -5.0f };
	GW::MATH::GVECTORF lookat = { 0.0f, 0.0f, 0.0f };
	GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	// Inverse to turn it into a camera matrix, not a view matrix. This will let us do
	// camera manipulation in the component easier
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(display,
		DRAW::Camera{ initialCamera });
}

entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName)
{
	// Create the entity
	entt::entity gameEntity = registry.create();

	// Add a MeshCollection component
	auto& meshCollection = registry.emplace<GAME::MeshCollection>(gameEntity);

	// Add a Transform component with identity matrix initially
	auto& transform = registry.emplace<GAME::Transform>(gameEntity);
	GW::MATH::GMatrix::IdentityF(transform.matrix);

	// Get the ModelManager
	auto& modelManager = registry.ctx().get<GAME::ModelManager>();

	// Check if the model collection exists
	std::cout << "Looking for model collection: " << modelName << std::endl;
	if (modelManager.collections.find(modelName) != modelManager.collections.end() &&
		!modelManager.collections[modelName].empty())
	{
		std::cout << "Found model collection: " << modelName << std::endl;

		// Get the entities from the collection
		auto& modelEntities = modelManager.collections[modelName];
		std::cout << "Model collection " << modelName << " has " << modelEntities.size() << " entities" << std::endl;

		// For each entity in the model collection
		for (auto modelEntity : modelEntities)
		{
			// Create a new entity for the mesh
			entt::entity meshEntity = registry.create();

			// Copy the GeometryData and GPUInstance components
			if (registry.all_of<DRAW::GeometryData>(modelEntity))
			{
				auto& geomData = registry.get<DRAW::GeometryData>(modelEntity);
				registry.emplace<DRAW::GeometryData>(meshEntity, geomData);
			}

			if (registry.all_of<DRAW::GPUInstance>(modelEntity))
			{
				auto& gpuInstance = registry.get<DRAW::GPUInstance>(modelEntity);
				registry.emplace<DRAW::GPUInstance>(meshEntity, gpuInstance);

				// Set the transform from the first entity in the collection
				if (modelEntities[0] == modelEntity)
				{
					transform.matrix = gpuInstance.transform;
				}
			}

			// Add the mesh entity to the game entity's MeshCollection
			meshCollection.meshEntities.push_back(meshEntity);
		}
	}
	else
	{
		std::cout << "Model collection not found or empty: " << modelName << std::endl;
	}

	return gameEntity;
}

void GameplayBehavior(entt::registry& registry)
{
	// Calculate delta time
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
	lastTime = currentTime;

	// Create GameManager entity if it doesn't exist
	static entt::entity gameManagerEntity = entt::null;
	if (gameManagerEntity == entt::null || !registry.valid(gameManagerEntity))
	{
		gameManagerEntity = registry.create();
		// Only add the GameManager component if it doesn't already exist
		if (!registry.all_of<GAME::GameManager>(gameManagerEntity)) {
			registry.emplace<GAME::GameManager>(gameManagerEntity);
			std::cout << "GameManager entity created" << std::endl;
		}
	}

	// Check if player and enemy entities exist
	static bool entitiesCreated = false;
	if (!entitiesCreated)
	{
		try {
			// Get the config file
			std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

			// Get model names from config with error checking
			std::string playerModelName = "Turtle"; // Default value
			std::string enemyModelName = "Cactus";  // Default value

			try {
				playerModelName = config->at("Player").at("model").as<std::string>();
			}
			catch (const std::exception& e) {
				std::cout << "Player model not found in config, using default: " << e.what() << std::endl;
				// Keep the default value
			}

			try {
				enemyModelName = config->at("Enemy1").at("model").as<std::string>();
			}
			catch (const std::exception& e) {
				std::cout << "Enemy model not found in config, using default: " << e.what() << std::endl;
				// Keep the default value
			}

			// Find the existing player entity instead of creating a new one
			auto playerView = registry.view<GAME::Player>();
			entt::entity playerEntity = entt::null;

			if (playerView.begin() != playerView.end()) {
				playerEntity = *playerView.begin();
				std::cout << "Found existing player entity" << std::endl;

				// Make sure the MeshCollection has a properly initialized collider
				if (registry.all_of<GAME::MeshCollection>(playerEntity)) {
					auto& meshCollection = registry.get<GAME::MeshCollection>(playerEntity);
					// Initialize the collider with default values - INCREASED SIZE
					meshCollection.collider.center = { 0.0f, 0.0f, 0.0f, 1.0f };
					meshCollection.collider.extent = { 1.5f, 1.5f, 1.5f, 1.0f }; // Larger size for better collision
					meshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
					std::cout << "Player collider initialized with size: " << meshCollection.collider.extent.x << std::endl;
				}

				// Only add the Collidable tag if it doesn't already exist
				if (!registry.all_of<GAME::Collidable>(playerEntity)) {
					registry.emplace<GAME::Collidable>(playerEntity);
					std::cout << "Player entity created with Collidable tag" << std::endl;
				}
			}
			else {
				std::cout << "No player entity found, this shouldn't happen" << std::endl;
			}

			// Create enemy entity
			entt::entity enemyEntity = GAME::CreateGameEntityFromModel(registry, enemyModelName);
			if (registry.valid(enemyEntity)) {
				// Only add the Enemy tag if it doesn't already exist
				if (!registry.all_of<GAME::Enemy>(enemyEntity)) {
					registry.emplace<GAME::Enemy>(enemyEntity);
				}

				// Make sure the MeshCollection has a properly initialized collider
				if (registry.all_of<GAME::MeshCollection>(enemyEntity)) {
					auto& meshCollection = registry.get<GAME::MeshCollection>(enemyEntity);
					// Initialize the collider with default values - INCREASED SIZE
					meshCollection.collider.center = { 0.0f, 0.0f, 0.0f, 1.0f };
					meshCollection.collider.extent = { 1.5f, 1.5f, 1.5f, 1.0f }; // Larger size for better collision
					meshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
					std::cout << "Enemy collider initialized with size: " << meshCollection.collider.extent.x << std::endl;
				}

				// Only add the Collidable tag if it doesn't already exist
				if (!registry.all_of<GAME::Collidable>(enemyEntity)) {
					registry.emplace<GAME::Collidable>(enemyEntity);
					std::cout << "Enemy entity created with Collidable tag" << std::endl;
				}

				// Add a Velocity component to the enemy with an initial direction and speed
				// Only add if it doesn't already exist
				if (!registry.all_of<GAME::Velocity>(enemyEntity)) {
					GW::MATH::GVECTORF enemyDirection = UTIL::GetRandomVelocityVector();
					float enemySpeed = 3.0f; // Default value
					try {
						std::string speedStr = config->at("Enemy1").at("speed").as<std::string>();
						enemySpeed = std::stof(speedStr);
					}
					catch (const std::exception& e) {
						std::cout << "Enemy speed not found in config, using default: " << e.what() << std::endl;
					}
					registry.emplace<GAME::Velocity>(enemyEntity, enemyDirection, enemySpeed);

					std::cout << "Enemy created with random diagonal direction: " << enemyDirection.x << ", "
						<< enemyDirection.z << " and speed: " << enemySpeed << std::endl;
				}
			}
			else {
				std::cout << "Failed to create enemy entity" << std::endl;
			}

			// Set initial visibility
			auto& gameManager = registry.ctx().get<GAME::GameManager>();
			if (registry.valid(playerEntity)) {
				GAME::SetEntityVisibility(registry, playerEntity, gameManager.playerVisible);
			}
			if (registry.valid(enemyEntity)) {
				GAME::SetEntityVisibility(registry, enemyEntity, gameManager.enemyVisible);
			}

			// Find and tag all wall entities with Obstacle and Collidable tags
			auto levelEntities = registry.view<GAME::MeshCollection>(entt::exclude<GAME::Player, GAME::Enemy, GAME::Bullet>);
			for (auto entity : levelEntities) {
				try {
					// Assuming walls are part of the level entities
					// Only add the Obstacle tag if it doesn't already exist
					if (!registry.all_of<GAME::Obstacle>(entity)) {
						registry.emplace<GAME::Obstacle>(entity);
					}

					// Only add the Collidable tag if it doesn't already exist
					if (!registry.all_of<GAME::Collidable>(entity)) {
						registry.emplace<GAME::Collidable>(entity);
					}

					// Initialize collider for the wall - INCREASED SIZE
					if (registry.all_of<GAME::MeshCollection>(entity)) {
						auto& meshCollection = registry.get<GAME::MeshCollection>(entity);
						// Initialize the collider with default values
						meshCollection.collider.center = { 0.0f, 0.0f, 0.0f, 1.0f };
						meshCollection.collider.extent = { 2.0f, 2.0f, 2.0f, 1.0f }; // Larger size for walls
						meshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
						std::cout << "Wall collider initialized with size: " << meshCollection.collider.extent.x << std::endl;
					}
					std::cout << "Tagged level entity as Obstacle and Collidable: " << (int)entity << std::endl;
				}
				catch (const std::exception& e) {
					std::cout << "Error tagging level entity: " << e.what() << std::endl;
				}
			}

			entitiesCreated = true;
		}
		catch (const std::exception& e) {
			std::cout << "Error in GameplayBehavior: " << e.what() << std::endl;
		}
	}

	// Update the GameManager
	GAME::UpdateGameManager(registry, deltaTime);
}

// This function will be called by the main loop to update the main loop
// It will be responsible for updating any created windows and handling any input
void MainLoopBehavior(entt::registry& registry)
{
	// main loop
	int closedCount; // count of closed windows
	auto winView = registry.view<APP::Window>(); // for updating all windows
	auto& deltaTime = registry.ctx().emplace<UTIL::DeltaTime>().dtSec;
	// for updating all windows
	do {
		// Set the delta time
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - start).count();
		start = std::chrono::steady_clock::now();
		// Cap delta time to min 30 fps. This will prevent too much time from simulating when dragging the window
		if (elapsed > 1.0 / 30.0)
		{
			elapsed = 1.0 / 30.0;
		}
		deltaTime = elapsed;

		// TODO : Update Game
		auto gameManagerView = registry.view<GAME::GameManager>();
		for (auto entity : gameManagerView) {
			registry.patch<GAME::GameManager>(entity); // Update the GameManager
		}

		closedCount = 0;
		// find all Windows that are not closed and call "patch" to update them
		for (auto entity : winView) {
			if (registry.any_of<APP::WindowClosed>(entity))
				++closedCount;
			else
				registry.patch<APP::Window>(entity); // calls on_update()
		}
	} while (winView.size() != closedCount); // exit when all windows are closed
}
