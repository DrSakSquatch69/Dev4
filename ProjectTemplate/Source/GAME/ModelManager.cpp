#include "ModelManager.h"
#include "GameManager.h"
#include "../DRAW/DrawComponents.h"
#include "../CCL.h"

namespace GAME {

    void InitializeModelManager(entt::registry& registry) {
        // Create a ModelManager in the registry context
        registry.ctx().emplace<ModelManager>();
        std::cout << "ModelManager initialized" << std::endl;
    }

    void AddEntityToCollection(entt::registry& registry, entt::entity entity, const std::string& collectionName) {
        // Get the ModelManager from the registry context
        auto& modelManager = registry.ctx().get<ModelManager>();
        
        // Add the entity to the named collection
        modelManager.collections[collectionName].push_back(entity);
        std::cout << "Added entity to collection: " << collectionName << std::endl;
    }

    std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName) {
        // Get the ModelManager from the registry context
        auto& modelManager = registry.ctx().get<ModelManager>();
        
        // Return the entities from the named collection
        if (modelManager.collections.find(collectionName) != modelManager.collections.end()) {
            return modelManager.collections[collectionName];
        }
        return std::vector<entt::entity>();
    }

    entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName) {
        // Create the entity
        entt::entity gameEntity = registry.create();

        // Add a MeshCollection component
        registry.emplace<MeshCollection>(gameEntity);

        // Add a Transform component with identity matrix initially
        auto& transform = registry.emplace<Transform>(gameEntity);
        GW::MATH::GMatrix::IdentityF(transform.matrix);

        // Get the ModelManager
        auto& modelManager = registry.ctx().get<ModelManager>();

        // Debug: Check if the model collection exists
        std::cout << "Looking for model collection: " << modelName << std::endl;
        if (modelManager.collections.find(modelName) != modelManager.collections.end()) {
            std::cout << "Found model collection: " << modelName << std::endl;

            // Get the entities from the collection
            auto modelEntities = GetEntitiesFromCollection(registry, modelName);
            std::cout << "Model collection " << modelName << " has " << modelEntities.size() << " entities" << std::endl;

            // For each entity in the model collection
            for (auto modelEntity : modelEntities) {
                // Create a new entity for the mesh
                entt::entity meshEntity = registry.create();

                // Copy the GeometryData and GPUInstance components
                if (registry.all_of<DRAW::GeometryData>(modelEntity)) {
                    auto& geomData = registry.get<DRAW::GeometryData>(modelEntity);
                    registry.emplace<DRAW::GeometryData>(meshEntity, geomData);
                }

                if (registry.all_of<DRAW::GPUInstance>(modelEntity)) {
                    auto& gpuInstance = registry.get<DRAW::GPUInstance>(modelEntity);
                    registry.emplace<DRAW::GPUInstance>(meshEntity, gpuInstance);

                    if (modelEntities[0] == modelEntity) {
                        transform.matrix = gpuInstance.transform; // Copy the entire transform
                    }
                }

                // Add the mesh entity to the game entity's MeshCollection
                auto& meshCollection = registry.get<MeshCollection>(gameEntity);
                meshCollection.meshEntities.push_back(meshEntity);
            }
        }
        else {
            std::cout << "Model collection not found: " << modelName << std::endl;
        }

        return gameEntity;
    }

} // namespace GAME