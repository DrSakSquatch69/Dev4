#include "Player.h"

namespace GAME
{
	// Update the player's position based on input 
	void UpdatePlayer(entt::registry& registry, entt::entity entity, float deltaTime, float speed) {
		// Get the input from the registry context 
		auto& input = registry.ctx().get<UTIL::Input>();

		// Get the player's transform 
		if (!registry.all_of(entity)) return;

		auto& transform = registry.get(entity);

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
			GW::MATH::GMatrix::TranslateGlobalF(transform, movement, transform);
			std::cout << "Player moved: " << movement.x << ", " << movement.z << std::endl;
		}
	}

	// on_update method for the Player component 
	void on_update(entt::registry& registry, entt::entity entity) {
		// Get the delta time from the registry context 
		auto& deltaTime = registry.ctx().getUTIL::DeltaTime().dtSec;

		// Get the config file for player speed 
		std::shared_ptr config = registry.ctx().getUTIL::Config().gameConfig;
		float playerSpeed = 5.0f; // Default value

		// Try to get the player speed from the config 
		try {
			playerSpeed = config->at("Player").at("speed").as();
		}
		catch (const std::exception& e) {
			std::cout << "Player speed not found in config, using default: " << e.what() << std::endl;
			// Keep the default value 
		}

		// Update the player 
		UpdatePlayer(registry, entity, static_cast(deltaTime), playerSpeed);
	}

	// Connect the Player logic to the registry 
	CONNECT_COMPONENT_LOGIC() {
		// Connect the on_update method for the Player component 
		registry.on_update().connect<&on_update>();
	}
}