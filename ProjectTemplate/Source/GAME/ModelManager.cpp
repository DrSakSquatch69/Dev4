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

} // namespace GAME