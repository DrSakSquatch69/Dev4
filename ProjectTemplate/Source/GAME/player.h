#ifndef PLAYER_H
#define PLAYER_H

#include "../CCL.h" 
#include "GameComponents.h" 
#include "../UTIL/Utilities.h"
#include "../UTIL/GameConfig.h"
#include <cmath> // For sqrt and normalization
#include "GameManager.h"


namespace GAME {
	// Update method for the Player component 
	void UpdatePlayer(entt::registry& registry, entt::entity entity, float deltaTime, float speed);

	// on_update method for the Player component 
	void player_on_update(entt::registry& registry, entt::entity entity);

	// Create a game entity from a model (declared in GameManager.cpp)
	entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName);
}

#endif // PLAYER_H