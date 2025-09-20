#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
    //*** Tags ***//
    struct Player {};      // Tag to identify player entity
    struct Enemy {};       // Tag to identify enemy entity
    struct Bullet {};      // Tag to identify bullet entity
    struct Wall {};        // Tag to identify wall entities
    struct Collidable {};  // Tag for entities that participate in collision detection

    //*** Components ***//
    struct Transform {
        GW::MATH::GMATRIXF matrix;
    };

    // Collection of mesh entities that make up a game entity
    struct MeshCollection {
        std::vector<entt::entity> meshEntities;
        GW::MATH::GOBBF collider;  // Oriented Bounding Box for collision detection
    };

    struct Firing {
        float cooldown;    // Current cooldown time remaining
        float maxCooldown; // Maximum cooldown time
    };

    // Velocity component for moving entities
    struct Velocity {
        GW::MATH::GVECTORF direction;  // Normalized direction vector
        float speed;                   // Speed in units per second
    };
}// namespace GAME
#endif // !GAME_COMPONENTS_H_
