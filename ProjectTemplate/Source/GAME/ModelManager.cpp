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