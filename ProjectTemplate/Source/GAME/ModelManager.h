#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "../DRAW/DrawComponents.h"
#include <string>
#include <map>
#include <vector>
#include "GameComponents.h"

namespace GAME {
    // Forward declarations
    struct Transform;
    struct MeshCollection;

    // ModelManager component to store model collections
    struct ModelManager {
        std::map<std::string, std::vector<entt::entity>> collections;
    };

    // Initialize the ModelManager
    void InitializeModelManager(entt::registry& registry);

    // Add an entity to a named collection
    void AddEntityToCollection(entt::registry& registry, entt::entity entity, const std::string& collectionName);

    // Get entities from a named collection
    std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName);

    // Create a game entity from a model
    entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName);
}

#endif // MODEL_MANAGER_H