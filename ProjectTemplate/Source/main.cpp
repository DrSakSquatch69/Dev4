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
using namespace GW::MATH;


// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);
void CreatePlayer(entt::registry& registry);
void CreateEnemy(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);
	GAME::InitializeModelManager(registry);


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

void CreatePlayer(entt::registry& registry, entt::entity playerEntity)
{
	// Always add required components
	if (!registry.all_of<GAME::Transform>(playerEntity)) {
		registry.emplace<GAME::Transform>(playerEntity);
	}
	if (!registry.all_of<GAME::MeshCollection>(playerEntity)) {
		registry.emplace<GAME::MeshCollection>(playerEntity);
	}
	auto& transform = registry.get<GAME::Transform>(playerEntity);
	
	// Add collider to player if it has a MeshCollection
	if (registry.all_of<GAME::MeshCollection>(playerEntity)) {
		auto& playerMeshCollection = registry.get<GAME::MeshCollection>(playerEntity);
		// Create a default player collider (you may want to load this from config)
		playerMeshCollection.collider.center = { 0.0f, 0.0f, 0.0f };
		playerMeshCollection.collider.extent = { 1.0f, 1.0f, 1.0f };
		playerMeshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		registry.emplace<GAME::Collidable>(playerEntity);
		std::cout << "Added collider to player entity" << std::endl;
		
	}
	
}

void CreateEnemy(entt::registry& registry, entt::entity enemyEntity) {
	// Always add required components (like CreatePlayer does)
	if (!registry.all_of<GAME::Transform>(enemyEntity)) {
		registry.emplace<GAME::Transform>(enemyEntity);
	}
	if (!registry.all_of<GAME::MeshCollection>(enemyEntity)) {
		registry.emplace<GAME::MeshCollection>(enemyEntity);
	}
	// Get the transform for positioning
	auto& transform = registry.get<GAME::Transform>(enemyEntity);

	if (registry.all_of<GAME::MeshCollection>(enemyEntity)) {
	auto& enemyMeshCollection = registry.get<GAME::MeshCollection>(enemyEntity);
	enemyMeshCollection.collider.center = { 0.0f, 0.0f, 0.0f }; // Keep this at origin
	enemyMeshCollection.collider.extent = { 1.0f, 1.0f, 1.0f }; // Increased size for better collision
	enemyMeshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };	
	registry.emplace_or_replace<GAME::Collidable>(enemyEntity);
	std::cout << "Added collider to enemy entity" << std::endl;
	}

	// Position the enemy somewhere visible
	transform.matrix.row4.x = -20.0f;  // Closer to left wall at -25
	transform.matrix.row4.z = -10.0f;  // Keep Z position reasonable

	// Use a controlled velocity toward the left wall
	GW::MATH::GVECTORF controlledDirection = { -1.0f, 0.0f, 0.0f }; // Moving left toward wall

	// Generate a random diagonal direction vector (ensures both X and Z are non-zero)
	GW::MATH::GVECTORF randomDirection = UTIL::GetRandomVelocityVector();

	// Add velocity with the random direction
	registry.emplace<GAME::Velocity>(enemyEntity, randomDirection, 3.0f);
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

void GameplayBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

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
		registry.emplace<GAME::GameManager>(gameManagerEntity);
		std::cout << "GameManager entity created" << std::endl;
	}
	// Get model names from config with error checking
	std::string playerModelName = "Turtle"; // Default value
	std::string enemyModelName = "Cactus";  // Default value

	try {
		enemyModelName = config->at("Enemy1").at("model").as<std::string>();
	}
	catch (const std::exception& e) {
		std::cout << "Enemy model not found in config, using default: " << e.what() << std::endl;
		// Keep the default value
	}


	try {
		playerModelName = config->at("Player").at("model").as<std::string>();
	}
	catch (const std::exception& e) {
		std::cout << "Player model not found in config, using default: " << e.what() << std::endl;
		// Keep the default value
	}

	std::cout << "Player model name: " << playerModelName << std::endl;
	std::cout << "Enemy model name: " << enemyModelName << std::endl;

	// Create enemy entity
	entt::entity enemyEntity = GAME::CreateGameEntityFromModel(registry, enemyModelName);
	registry.emplace<GAME::Enemy>(enemyEntity);
	std::cout << "Enemy entity created" << std::endl;

	// Create player entity
	entt::entity playerEntity = GAME::CreateGameEntityFromModel(registry, playerModelName);
	registry.emplace<GAME::Player>(playerEntity);
	std::cout << "Player entity created" << std::endl;
	
	CreatePlayer(registry, playerEntity);
	CreateEnemy(registry, enemyEntity);

	// Set initial visibility
	auto& gameManager = registry.ctx().get<GAME::GameManager>();
	GAME::SetEntityVisibility(registry, playerEntity, gameManager.playerVisible);
	GAME::SetEntityVisibility(registry, enemyEntity, gameManager.enemyVisible);

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
