#include "CollisionHelper.h"
#include "GameComponents.h"
#include "../CCL.h"

namespace GAME {
    // This function will be called when a Player component is added to an entity
    void on_player_construct(entt::registry& registry, entt::entity entity) {
        // Make the player entity collidable
        MakeEntityCollidable(registry, entity);
        std::cout << "Player entity is now collidable" << std::endl;
    }

    // This function will be called when an Enemy component is added to an entity
    void on_enemy_construct(entt::registry& registry, entt::entity entity) {
        // Make the enemy entity collidable
        MakeEntityCollidable(registry, entity);
        std::cout << "Enemy entity is now collidable" << std::endl;
    }

    // This function will be called when a Bullet component is added to an entity
    void on_bullet_construct(entt::registry& registry, entt::entity entity) {
        // Make the bullet entity collidable
        MakeEntityCollidable(registry, entity);
        std::cout << "Bullet entity is now collidable" << std::endl;
    }

    // Connect the collision setup logic to the registry
    CONNECT_COMPONENT_LOGIC() {
        // Connect the on_construct methods for the Player, Enemy, and Bullet components
        registry.on_construct<Player>().connect<&on_player_construct>();
        registry.on_construct<Enemy>().connect<&on_enemy_construct>();
        registry.on_construct<Bullet>().connect<&on_bullet_construct>();
    }
}