#include "ModelManager.h"
#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../CCL.h"

namespace GAME {

    void InitializeModelManager(entt::registry& registry) { // Create a ModelManager in the registry context 
        registry.ctx().emplace<ModelManager>();
    }

    void AddEntityToCollection(entt::registry& registry, entt::entity entity, const std::string& collectionName) {
        std::cout << "Adding entity " << static_cast<uint32_t>(entity) << " to collection: " << collectionName << std::endl;

        // Get the ModelManager from the registry context 
        auto& modelManager = registry.ctx().get<ModelManager>();

        // Check if the collection exists 
        if (modelManager.collections.find(collectionName) == modelManager.collections.end()) {
            std::cout << "Creating new collection: " << collectionName << std::endl;
            // Create a new collection 
            MeshCollection newCollection;
            newCollection.name = collectionName;
            modelManager.collections[collectionName] = newCollection;
        }

        // Add the entity to the collection 
        modelManager.collections[collectionName].meshEntities.push_back(entity);
        std::cout << "Collection " << collectionName << " now has " << modelManager.collections[collectionName].meshEntities.size() << " entities" << std::endl;
    }

    std::vector<entt::entity> GetEntitiesFromCollection(entt::registry& registry, const std::string& collectionName) {
        // Get the ModelManager from the registry context 
        auto& modelManager = registry.ctx().get<ModelManager>();

        // Check if the collection exists 
        if (modelManager.collections.find(collectionName) != modelManager.collections.end()) {
            // Return the entities in the collection 
            return modelManager.collections[collectionName].meshEntities;
        }

        // Return an empty vector if the collection doesn't exist 
        std::vector<entt::entity> returnVec;
        return returnVec;

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
                        // Copy only the position from the original transform, not the rotation
                        transform.matrix = GW::MATH::GIdentityMatrixF;
                        transform.matrix.row4 = gpuInstance.transform.row4; // Copy only position
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

    // Function to clean up ModelManager when it's destroyed 
    void Destroy_ModelManager(entt::registry& registry) {
        // Clear all collections 
        auto& modelManager = registry.ctx().get<ModelManager>();
        modelManager.collections.clear();
    }

    // Connect the ModelManager logic to the registry 
    CONNECT_COMPONENT_LOGIC() {
        // Initialize the ModelManager 
        InitializeModelManager(registry);
    }

} // namespace GAME