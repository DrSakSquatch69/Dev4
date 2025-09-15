#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
    //*** Tags ***//
    struct Player {};      // Tag to identify player entity
    struct Enemy {};       // Tag to identify enemy entity
    struct Bullet {};      // Tag to identify bullet entity
    struct Collidable {};  // Tag to identify collidable entity
    struct Obstacle {};    // Tag to identify wall/obstacle entity
    struct ToDestroy {};   // Tag to identify entities that should be destroyed

    //*** Components ***//
    struct Transform {
        GW::MATH::GMATRIXF matrix;
    };

    // Collection of mesh entities that make up a game entity
    struct MeshCollection {
        std::vector<entt::entity> meshEntities;
        GW::MATH::GOBBF obb;
    };

    struct Firing {
        float cooldown;    // Current cooldown time remaining
        float maxCooldown; // Maximum cooldown time
    };

    // Velocity component for movement
    struct Velocity {
        GW::MATH::GVECTORF direction; // Normalized direction vector
        float speed;                  // Speed scalar
    };

    // Shatters component for enemy shattering behavior
    struct Shatters {
        int shatterCount;   // How many more times this entity can shatter
        int shatterAmount;  // How many pieces to shatter into
        float shatterScale; // Scale factor for shattered pieces
    };
}// namespace GAME
#endif // !GAME_COMPONENTS_H_