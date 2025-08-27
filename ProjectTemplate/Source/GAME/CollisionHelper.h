#ifndef COLLISION_HELPER_H
#define COLLISION_HELPER_H

#include "../CCL.h"
#include "GameComponents.h"

namespace GAME {
    // Add the Collidable tag to an entity and set up its collider
    inline void MakeEntityCollidable(entt::registry& registry, entt::entity entity) {
        if (registry.valid(entity) && registry.all_of<MeshCollection>(entity)) {
            // Add the Collidable tag
            registry.emplace_or_replace<Collidable>(entity);
            
            // Set up a default collider if needed
            auto& meshCollection = registry.get<MeshCollection>(entity);
            
            // Only set up the collider if it's not already set up
            if (meshCollection.collider.extent.x == 0.0f && 
                meshCollection.collider.extent.y == 0.0f && 
                meshCollection.collider.extent.z == 0.0f) {
                
                // Set up a default collider
                meshCollection.collider.center = { 0.0f, 0.0f, 0.0f, 1.0f };
                meshCollection.collider.extent = { 1.0f, 1.0f, 1.0f, 0.0f }; // Default size
                meshCollection.collider.orientation = { 0.0f, 0.0f, 0.0f, 1.0f }; // No rotation
            }
        }
    }
}

#endif // COLLISION_HELPER_H