#include "Player.h"
using namespace GW::MATH;

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
		if (registry.all_of<Invulnerability>(entity)) {
			// Reduce the cooldown
			auto& invuln = registry.get<Firing>(entity);
			invuln.cooldown -= deltaTime;

			// If cooldown reaches 0 or below, remove the Firing component
			if (invuln.cooldown <= 0.0f) {
				registry.remove<Invulnerability>(entity);
				std::cout << "Firing cooldown complete, ready to fire again" << std::endl;
			}
		}
		// Handle firing logic
		// First check if the player has a Firing component
		if (registry.all_of<Firing>(entity)) {
			// Reduce the cooldown
			auto& firing = registry.get<Firing>(entity);
			firing.cooldown -= deltaTime;

			// If cooldown reaches 0 or below, remove the Firing component
			if (firing.cooldown <= 0.0f) {
				registry.remove<Firing>(entity);
				std::cout << "Firing cooldown complete, ready to fire again" << std::endl;
			}
		}
		else {
			// Check for firing keys
			float upKey = 0.0f, downKey = 0.0f, leftKey = 0.0f, rightKey = 0.0f;
			input.immediateInput.GetState(G_KEY_UP, upKey);
			input.immediateInput.GetState(G_KEY_DOWN, downKey);
			input.immediateInput.GetState(G_KEY_LEFT, leftKey);
			input.immediateInput.GetState(G_KEY_RIGHT, rightKey);

			// If any firing key is pressed, create a bullet and add the Firing component
			if (upKey > 0.0f || downKey > 0.0f || leftKey > 0.0f || rightKey > 0.0f) {
				// Create a bullet entity using the GAME namespace version of CreateGameEntityFromModel
				entt::entity bulletEntity = GAME::CreateGameEntityFromModel(registry, "Bullet");
				// Add collider to bullet if it has a MeshCollection
				if (registry.all_of<GAME::MeshCollection>(bulletEntity)) {
					auto& bulletMeshCollection = registry.get<GAME::MeshCollection>(bulletEntity);
					// Create a small bullet collider
					bulletMeshCollection.collider.extent = { 0.2f, 0.2f, 0.2f };
					bulletMeshCollection.collider.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

					// Set the collider center to match the bullet's actual position
					auto& bulletTransform = registry.get<Transform>(bulletEntity);
					GVECTORF bulletPos;
					bulletPos.x = bulletTransform.matrix.data[12];
					bulletPos.y = bulletTransform.matrix.data[13];
					bulletPos.z = bulletTransform.matrix.data[14];
					bulletPos.w = 1.0f;
					bulletMeshCollection.collider.center = bulletPos;

					registry.emplace<GAME::Collidable>(bulletEntity);
					std::cout << "Added collider to bullet entity at position: ("
						<< bulletPos.x << ", " << bulletPos.y << ", " << bulletPos.z << ")" << std::endl;
				}
				// Add the Bullet tag
				registry.emplace<Bullet>(bulletEntity);

				// Set the bullet's position to the player's position
				auto& bulletTransform = registry.get<Transform>(bulletEntity);
				bulletTransform.matrix = transform.matrix; // Copy the player's transform

				// Create directional velocity based on arrow key input
				GW::MATH::GVECTORF direction = { 0.0f, 0.0f, 0.0f };

				// Build direction vector based on which arrow keys are pressed
				if (upKey > 0.0f) { direction.z += 1.0f; }    // Forward
				if (downKey > 0.0f) { direction.z -= 1.0f; }  // Backward
				if (leftKey > 0.0f) { direction.x -= 1.0f; }  // Left
				if (rightKey > 0.0f) { direction.x += 1.0f; } // Right

				// Normalize the direction vector if moving diagonally to maintain consistent speed
				if (direction.x != 0.0f && direction.z != 0.0f) {
					float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
					direction.x /= length; direction.z /= length;
				}

				// Get bullet speed from tuning file
				std::shared_ptr config = registry.ctx().get<UTIL::Config>().gameConfig;
				float bulletSpeed = 20.0f; // Default value from defaults.ini

				try {
					std::string speedStr = config->at("Bullet").at("speed").as<std::string>();
					bulletSpeed = std::stof(speedStr);
				}
				catch (const std::exception& e) {
					std::cout << "Bullet speed not found in config, using default: " << e.what() << std::endl;
					// Keep the default value
				}
				
				// Apply the directional velocity to the bullet
				registry.emplace<Velocity>(bulletEntity, direction, bulletSpeed);

				// Add the Firing component to the player with a cooldown
				registry.emplace<Firing>(entity, 0.5f, 0.5f); // 0.5 seconds cooldown

				std::cout << "Bullet fired!" << std::endl;
			}
		}
	}

	// on_update method for the Player component 
	void player_on_update(entt::registry& registry, entt::entity entity) {
		float deltaTime = 0.016f; // Default to 60fps (1/60 = 0.016) if DeltaTime not available

		// Safely check if DeltaTime exists in the registry context
		try {
			if (registry.ctx().contains<UTIL::DeltaTime>()) {
				deltaTime = static_cast<float>(registry.ctx().get<UTIL::DeltaTime>().dtSec);
			}
		}
		catch (const std::exception& e) {
			std::cout << "DeltaTime not available, using default: " << e.what() << std::endl;
			// Keep using the default value
		}
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