#ifndef MODEL_MANAGER_H_
#define MODEL_MANAGER_H_

#include "../../entt-3.13.1/single_include/entt/entt.hpp"
#include "GameComponents.h" // Include this to access MeshCollection
#include <string>
#include <vector>
#include <map>

namespace GAME {
    // ModelManager component to manage mesh collections 
    struct ModelManager {
        std::map<std::string, MeshCollection> collections;
    };

    // Initialize the ModelManager 
    void InitializeModelManager(entt::registry& registry);

    // Add an entity to a collection 
    void AddEntityToCollection(entt::registry& registry, entt::entity entity, const std::string& collectionName);

    // Get entities from a collection 
    std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName);

    // Create a game entity from a model collection
    entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName);


} // namespace GAME

#endif // !MODEL_MANAGER_H_