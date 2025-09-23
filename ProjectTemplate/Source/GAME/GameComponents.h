#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
    //*** Tags ***//
    struct Player {};      // Tag to identify player entity
    struct Enemy {};       // Tag to identify enemy entity
    struct Bullet {};      // Tag to identify bullet entity
	struct Collidable {};  // Tag to identify entities that can collide
	struct Obstacle {};   // Tag to identify static obstacles
	struct toDestroy {};   // Tag to mark entities for destruction

    //*** Components ***//
    struct Transform {
        GW::MATH::GMATRIXF matrix;
    };

    // Collection of mesh entities that make up a game entity
    struct MeshCollection {
        std::vector<entt::entity> meshEntities;
        GW::MATH::GOBBF collider; // OBB collider for the entire model
    };

    struct Firing {
        float cooldown;    // Current cooldown time remaining
        float maxCooldown; // Maximum cooldown time
    };

    struct Velocity {
        GW::MATH::GVECTORF direction;  // Which way to move (like a compass direction)
        float speed;                   // How fast to move (like 10 units per second)
    };

}// namespace GAME
#endif // !GAME_COMPONENTS_H_