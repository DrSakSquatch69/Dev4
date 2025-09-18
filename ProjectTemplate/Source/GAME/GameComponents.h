#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
    //*** Tags ***//
    struct Player {};      // Tag to identify player entity
    struct Enemy {};       // Tag to identify enemy entity
    struct Bullet {};      // Tag to identify bullet entity
    struct Collidable {};  // Tag for entities that participate in collision detection
    struct ToDestroy {};  // Tag for entities that should be removed from the game

    //*** Components ***//
    struct Transform {
        GW::MATH::GMATRIXF matrix;
    };

    // Collection of mesh entities that make up a game entity
    struct MeshCollection {
        std::vector<entt::entity> meshEntities;
    };

    struct Firing {
        float cooldown;    // Current cooldown time remaining
        float maxCooldown; // Maximum cooldown time
    };

    struct Velocity {
        GW::MATH::GVECTORF direction;  // Normalized direction vector
        float speed;                   // Speed in units per second
    };

    struct Shatters {
        int count;         // Number of times this entity can shatter
        int amount;        // Number of pieces to create when shattered
        float scale;       // Scale factor for shattered pieces
    };

}// namespace GAME
#endif // !GAME_COMPONENTS_H_