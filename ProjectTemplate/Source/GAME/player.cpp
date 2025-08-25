#include "Player.h"

namespace GAME
{
	// Update the player's position based on input 
	void UpdatePlayer(entt::registry& registry, entt::entity entity, float deltaTime, float speed) {
		// Get the input from the registry context 
		auto& input = registry.ctx().get<UTIL::Input>();

		// Get the player's transform 
		if (!registry.all_of<GAME::Transform>(entity)) return;
		auto& transform = registry.get<GAME::Transform>(entity);

		// Check for WASD key input 
		float wKey = 0.0f, aKey = 0.0f, sKey = 0.0f, dKey = 0.0f;
		input.immediateInput.GetState(G_KEY_W, wKey);
		input.immediateInput.GetState(G_KEY_A, aKey);
		input.immediateInput.GetState(G_KEY_S, sKey);
		input.immediateInput.GetState(G_KEY_D, dKey);

		// Movement vector (on X/Z plane) 
		GW::MATH::GVECTORF movement = { 0.0f, 0.0f, 0.0f };

		// Apply movement based on key states 
		if (dKey > 0.0f) { movement.x += 1.0f; }
		if (aKey > 0.0f) { movement.x -= 1.0f; }
		if (wKey > 0.0f) { movement.z += 1.0f; }
		if (sKey > 0.0f) { movement.z -= 1.0f; }

		// Normalize the movement vector if moving diagonally to maintain consistent speed 
		if (movement.x != 0.0f && movement.z != 0.0f) {
			float length = std::sqrt(movement.x * movement.x + movement.z * movement.z);
			movement.x /= length; movement.z /= length;
		}

		// Apply speed and delta time to movement 
		movement.x *= speed * deltaTime;
		movement.z *= speed * deltaTime;

		// Apply movement to transform if there is any movement 
		if (movement.x != 0.0f || movement.z != 0.0f) {
			GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, transform.matrix);
			std::cout << "Player moved: " << movement.x << ", " << movement.z << std::endl;
		}
	}

	// on_update method for the Player component 
	void player_on_update(entt::registry& registry, entt::entity entity) {
		// Get the delta time from the registry context 
		auto& deltaTimeComponent = registry.ctx().get<UTIL::DeltaTime>();
		float deltaTime = deltaTimeComponent.dtSec;

		// Get the config file for player speed 
		std::shared_ptr config = registry.ctx().get<UTIL::Config>().gameConfig;

		float playerSpeed = 5.0f; // Default value

		// Try to get the player speed from the config 

		try {
			// Try to get the value as a string and convert to float
			std::string speedStr = config->at("Player").at("speed").as<std::string>();
			playerSpeed = std::stof(speedStr);
		}
		catch (const std::exception& e) {
			std::cout << "Player speed not found in config, using default: " << e.what() << std::endl;
			// Keep the default value
		}

		// Update the player 
		UpdatePlayer(registry, entity, deltaTime, playerSpeed);
	}

	// Connect the Player logic to the registry 
	CONNECT_COMPONENT_LOGIC() {
		// Connect the on_update method for the Player component 
		registry.on_update<Player>().connect<&player_on_update>();
	}
}