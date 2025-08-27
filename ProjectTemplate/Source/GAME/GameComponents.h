#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	//*** Tags ***//
	struct Player {};      // Tag to identify player entity
	struct Enemy {};       // Tag to identify enemy entity
	struct Bullet {};      // Tag to identify bullet entity
	struct Collidable {};  // Tag to identify entities that can collide
	struct Obstacle {};    // Tag to identify obstacles like walls


	//*** Components ***//
	struct Transform {
		GW::MATH::GMATRIXF matrix;
	};

	struct Firing {
		float cooldown;    // Current cooldown time remaining
		float maxCooldown; // Maximum cooldown time
	};

	struct Velocity {
		GW::MATH::GVECTORF direction; // Direction of movement
		float speed;                  // Speed scalar
	};

	struct MeshCollection {
		std::vector<entt::entity> meshEntities;
		GW::MATH::GOBBF collider; // Oriented Bounding Box for collision detection
	};


}// namespace GAME
#endif // !GAME_COMPONENTS_H_