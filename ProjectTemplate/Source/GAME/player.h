#ifndef PLAYER_H
#define PLAYER_H

#include "../CCL.h" 
#include "GameComponents.h" 
#include "../UTIL/Utilities.h"

namespace GAME {
	// Update method for the Player component 
	void UpdatePlayer(entt::registry& registry, entt::entity entity, float deltaTime, float speed);

	// on_update method for the Player component 
	void on_update(entt::registry& registry, entt::entity entity);
}

#endif // PLAYER_H