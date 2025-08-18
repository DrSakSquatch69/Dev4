// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "APP/Window.hpp"
#include "GAME/ModelManager.h"
#include "GAME/GameManager.h"


// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;	

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);

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
	registry.emplace<DRAW::CPULevel>(display, DRAW::CPULevel{LevelFile, ModelPath});

	// Emplace and initialize Window component
	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();
	registry.emplace<APP::Window>(display,
		APP::Window{ startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, "Jacob Blackburn - Assignment 2"});


	// Create the input
	auto& input =  registry.ctx().emplace<UTIL::Input>();
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
	GW::MATH::GVECTORF translate = { 0.0f, 0.0f, -45.0f }; // Position back from the scene
	GW::MATH::GVECTORF lookat = { 0.0f, 0.0f, 0.0f }; // Looking at the center
	GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f }; // Up vector pointing up in world space
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	// Inverse to turn it into a camera matrix, not a view matrix. This will let us do
	// camera manipulation in the component easier
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(display,
		DRAW::Camera{ initialCamera });

	GAME::InitializeModelManager(registry);
}

// This function will be called by the main loop to update the gameplay
// It will be responsible for updating the VulkanInstances and any other gameplay components
void GameplayBehavior(entt::registry& registry)
{
	// Get the config
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	// Debug: Check if ModelManager has collections
	auto& modelManager = registry.ctx().get<GAME::ModelManager>();
	std::cout << "ModelManager collections count: " << modelManager.collections.size() << std::endl;

	// Debug: List all collections in ModelManager
	std::cout << "Available collections: ";
	for (const auto& [name, collection] : modelManager.collections) {
		std::cout << name << "(" << collection.meshEntities.size() << " entities), ";
	}
	std::cout << std::endl;

	// Get model names from config
	std::string playerModelName = (*config).at("Player").at("model").as<std::string>();
	std::string enemyModelName = (*config).at("Enemy1").at("model").as<std::string>();

	std::cout << "Player model name from config: " << playerModelName << std::endl;
	std::cout << "Enemy model name from config: " << enemyModelName << std::endl;

	// Create the Player entity
	entt::entity playerEntity = GAME::CreateGameEntityFromModel(registry, playerModelName);
	registry.emplace<GAME::Player>(playerEntity); // Add Player tag

	// Debug: Check Player mesh collection
	auto& playerMeshes = registry.get<GAME::MeshCollection>(playerEntity);
	std::cout << "Player has " << playerMeshes.meshEntities.size() << " mesh entities" << std::endl;

	// Create the Enemy entity
	entt::entity enemyEntity = GAME::CreateGameEntityFromModel(registry, enemyModelName);
	registry.emplace<GAME::Enemy>(enemyEntity); // Add Enemy tag

	// Debug: Check Enemy mesh collection
	auto& enemyMeshes = registry.get<GAME::MeshCollection>(enemyEntity);
	std::cout << "Enemy has " << enemyMeshes.meshEntities.size() << " mesh entities" << std::endl;

	std::cout << "Created Player and Enemy entities with meshes from models: "
		<< playerModelName << " and " << enemyModelName << std::endl;
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
		if(elapsed > 1.0 / 30.0)
		{
			elapsed = 1.0 / 30.0;
		}
		deltaTime = elapsed;

		// Update Game using GameManager
                GAME::UpdateGameManager(registry, deltaTime);

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
