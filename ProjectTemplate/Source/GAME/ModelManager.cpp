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
    
    // Register a model with the ModelManager
        // Get the ModelManager from the registry context
        auto& modelManager = registry.ctx().get<ModelManager>();
        
        // Create a new entity for the model
        entt::entity modelEntity = registry.create();
        
        // Add the model entity to the collection
        modelManager.collections[modelName].push_back(modelEntity);
        
        // Add a GeometryData component to the entity
        DRAW::GeometryData geomData;
        geomData.indexStart = 0;
        geomData.indexCount = 0;
        geomData.vertexStart = 0;
        registry.emplace<DRAW::GeometryData>(modelEntity, geomData);
        
        // Add a GPUInstance component to the entity
        DRAW::GPUInstance instance;
        GW::MATH::GMatrix::IdentityF(instance.transform);
        registry.emplace<DRAW::GPUInstance>(modelEntity, instance);
        
        std::cout << "Registered model: " << modelName << " at path: " << modelPath << std::endl;
    }
    
    // Register all required models
    void RegisterRequiredModels(entt::registry& registry) {
        // Get the config file
        std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
        
        // Get the model path
        std::string modelPath = "../Models";
        try {
            modelPath = (*config).at("Level1").at("modelPath").as<std::string>();
        }
        catch (const std::exception& e) {
            std::cout << "Model path not found in config, using default: " << e.what() << std::endl;
        }
        
        // Register the player model
        std::string playerModel = "Turtle";
        try {
            playerModel = (*config).at("Player").at("model").as<std::string>();
        }
        catch (const std::exception& e) {
            std::cout << "Player model not found in config, using default: " << e.what() << std::endl;
        }
        RegisterModel(registry, playerModel, modelPath + "/" + playerModel + ".h2b");
        
        // Register the enemy model
        std::string enemyModel = "Cactus";
        try {
            enemyModel = (*config).at("Enemy1").at("model").as<std::string>();
        }
        catch (const std::exception& e) {
            std::cout << "Enemy model not found in config, using default: " << e.what() << std::endl;
        }
        RegisterModel(registry, enemyModel, modelPath + "/" + enemyModel + ".h2b");
        
        // Register the bullet model
        std::string bulletModel = "Bullet";
        try {
            bulletModel = (*config).at("Bullet").at("model").as<std::string>();
        }
        catch (const std::exception& e) {
            std::cout << "Bullet model not found in config, using default: " << e.what() << std::endl;
        }
        RegisterModel(registry, bulletModel, modelPath + "/" + bulletModel + ".h2b");
        
        std::cout << "All required models registered" << std::endl;
    }

} // namespace GAME
// Register a model with the ModelManager
void RegisterModel(entt::registry& registry, const std::string& modelName, const std::string& modelPath) {
    // Get the ModelManager from the registry context
    auto& modelManager = registry.ctx().get<ModelManager>();
    
    // Create a new entity for the model
    entt::entity modelEntity = registry.create();
    
    // Add the model entity to the collection
    modelManager.collections[modelName].push_back(modelEntity);
    
    // Add a GeometryData component to the entity
    DRAW::GeometryData geomData;
    geomData.indexStart = 0;
    geomData.indexCount = 36; // Default to a cube (6 faces * 2 triangles * 3 vertices)
    geomData.vertexStart = 0;
    registry.emplace<DRAW::GeometryData>(modelEntity, geomData);
    
    // Add a GPUInstance component to the entity
    DRAW::GPUInstance instance;
    GW::MATH::GMatrix::IdentityF(instance.transform);
    registry.emplace<DRAW::GPUInstance>(modelEntity, instance);
    
    // Create a simple OBB for collision detection
    GW::MATH::GOBBF obb;
    obb.center = { 0.0f, 0.0f, 0.0f, 1.0f };
    obb.extents = { 0.5f, 0.5f, 0.5f, 0.0f }; // Half-size of the model
    obb.orientation = { 1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };
    
    // Store the OBB in the model manager for later use
    modelManager.modelOBBs[modelName] = obb;
    
    std::cout << "Registered model: " << modelName << " at path: " << modelPath << std::endl;
}

// Create a game entity from a model with proper OBB
entt::entity CreateGameEntityFromModel(entt::registry& registry, const std::string& modelName) {
    // Create the entity
    entt::entity gameEntity = registry.create();

    // Add a MeshCollection component
    auto& meshCollection = registry.emplace<GAME::MeshCollection>(gameEntity);

    // Add a Transform component with identity matrix initially
    auto& transform = registry.emplace<GAME::Transform>(gameEntity);
    GW::MATH::GMatrix::IdentityF(transform.matrix);

    // Get the ModelManager
    auto& modelManager = registry.ctx().get<GAME::ModelManager>();

    // Check if the model collection exists
    std::cout << "Looking for model collection: " << modelName << std::endl;
    if (modelManager.collections.find(modelName) != modelManager.collections.end() &&
        !modelManager.collections[modelName].empty())
    {
        std::cout << "Found model collection: " << modelName << std::endl;

        // Get the entities from the collection
        auto& modelEntities = modelManager.collections[modelName];
        std::cout << "Model collection " << modelName << " has " << modelEntities.size() << " entities" << std::endl;

        // For each entity in the model collection
        for (auto modelEntity : modelEntities)
        {
            // Create a new entity for the mesh
            entt::entity meshEntity = registry.create();

            // Copy the GeometryData and GPUInstance components
            if (registry.all_of<DRAW::GeometryData>(modelEntity))
            {
                auto& geomData = registry.get<DRAW::GeometryData>(modelEntity);
                registry.emplace<DRAW::GeometryData>(meshEntity, geomData);
            }

            if (registry.all_of<DRAW::GPUInstance>(modelEntity))
            {
                auto& gpuInstance = registry.get<DRAW::GPUInstance>(modelEntity);
                registry.emplace<DRAW::GPUInstance>(meshEntity, gpuInstance);

                // Set the transform from the first entity in the collection
                if (modelEntities[0] == modelEntity)
                {
                    transform.matrix = gpuInstance.transform;
                }
            }

            // Add the mesh entity to the game entity's MeshCollection
            meshCollection.meshEntities.push_back(meshEntity);
        }
        
        // Set the OBB for collision detection
        if (modelManager.modelOBBs.find(modelName) != modelManager.modelOBBs.end()) {
            meshCollection.obb = modelManager.modelOBBs[modelName];
        } else {
            // Default OBB if not found
            meshCollection.obb.center = { 0.0f, 0.0f, 0.0f, 1.0f };
            meshCollection.obb.extents = { 0.5f, 0.5f, 0.5f, 0.0f };
            meshCollection.obb.orientation = { 1.0f, 0.0f, 0.0f, 0.0f,
                                              0.0f, 1.0f, 0.0f, 0.0f,
                                              0.0f, 0.0f, 1.0f, 0.0f,
                                              0.0f, 0.0f, 0.0f, 1.0f };
        }
    }
    else
    {
        std::cout << "Model collection not found or empty: " << modelName << std::endl;
    }

    return gameEntity;
}