#include "DrawComponents.h"
#include "../CCL.h"

namespace DRAW
{
// Call this function after Level_Data is loaded and buffers are ready
void BuildLevelEntities(entt::registry& registry, entt::entity displayEntity)
    {
        // Get the CPULevel and Level_Data
        if (!registry.all_of<CPULevel>(displayEntity)) return;
        auto& cpuLevel = registry.get<CPULevel>(displayEntity);
        Level_Data& levelData = cpuLevel.lvlData;

        // For each blender object in the level
        for (const auto& blenderObj : levelData.blenderObjects)
        {
            // Get the model and transform for this object
            if (blenderObj.modelIndex >= levelData.levelModels.size()) continue;
            const auto& model = levelData.levelModels[blenderObj.modelIndex];

            // Each model can have multiple meshes
            for (unsigned meshIdx = 0; meshIdx < model.meshCount; ++meshIdx)
            {
                // Get the mesh
                unsigned meshGlobalIdx = model.meshStart + meshIdx;
                if (meshGlobalIdx >= levelData.levelMeshes.size()) continue;
                const auto& mesh = levelData.levelMeshes[meshGlobalIdx];

                // Create a new entity for this mesh instance
                entt::entity meshEntity = registry.create();

                // Add the entity to the appropriate collection based on the model name
                std::string modelName = model.filename;
                std::string collectionName = modelName;
                size_t lastSlash = collectionName.find_last_of("/\&quot;);
                    if (lastSlash != std::string::npos)
                        collectionName = collectionName.substr(lastSlash + 1);

                size_t lastDot = collectionName.find_last_of(".");
                if (lastDot != std::string::npos)
                    collectionName = collectionName.substr(0, lastDot);

                std::cout << "Adding entity to collection: " << collectionName << std::endl;
                GAME::AddEntityToCollection(registry, meshEntity, collectionName);

                // Fill out GeometryData
                GeometryData geom;
                geom.indexStart = model.indexStart + mesh.drawInfo.indexOffset;
                geom.indexCount = mesh.drawInfo.indexCount;
                geom.vertexStart = model.vertexStart;

                // Fill out GPUInstance
                GPUInstance instance;
                // Get the transform for this blender object
                if (blenderObj.transformIndex < levelData.levelTransforms.size())
                    instance.transform = levelData.levelTransforms[blenderObj.transformIndex];
                else
                    instance.transform = GW::MATH::GIdentityMatrixF;
                // Get the material for this mesh
                if (mesh.materialIndex + model.materialStart < levelData.levelMaterials.size())
                    instance.matData = levelData.levelMaterials[mesh.materialIndex + model.materialStart].attrib;
                else
                    instance.matData = {};

                // Attach components
                registry.emplace<GeometryData>(meshEntity, geom);
                registry.emplace<GPUInstance>(meshEntity, instance);

                // Add DoNotRender tag to dynamic meshes
                if (model.isDynamic)
                {
                    registry.emplace<DoNotRender>(meshEntity);
                    std::cout << "Added DoNotRender tag to dynamic model mesh" << model.filename << std::endl;
                }
                else
                {
                    std::cout << "Static model (should render): " << model.filename << std::endl;
                }
            }
        }
}

void Construct_CPULevel(entt::registry& registry, entt::entity entity)
{
	auto& cpuLevel = registry.get<CPULevel>(entity);

	GW::SYSTEM::GLog log;
	log.Create("LevelLoadLog");
	log.EnableConsoleLogging(true);

	cpuLevel.lvlData.LoadLevel(cpuLevel.levelFile.c_str(), 
        cpuLevel.modelPath.c_str(), 
        log);
}

void Construct_GPULevel(entt::registry& registry, entt::entity entity)
{
    // Make sure we have a CPULevel to get data from
    if (!registry.all_of<CPULevel>(entity))
        return;

    auto& cpuLevel = registry.get<CPULevel>(entity);
    auto& gpuLevel = registry.get<GPULevel>(entity);

    // Create vertex and index buffers if they don't exist
    if (!registry.all_of<VulkanVertexBuffer>(entity))
        registry.emplace<VulkanVertexBuffer>(entity);

    if (!registry.all_of<VulkanIndexBuffer>(entity))
        registry.emplace<VulkanIndexBuffer>(entity);

    // Build level entities using the correct approach from Assignment 1, step 3A
    BuildLevelEntities(registry, entity);

    // Emplace vertex data from CPULevel
    registry.emplace<std::vector<H2B::VERTEX>>(entity, cpuLevel.lvlData.levelVertices);

    // Update the vertex buffer to load data to GPU
    registry.patch<VulkanVertexBuffer>(entity);

    // Emplace index data from CPULevel
    registry.emplace<std::vector<unsigned int>>(entity, cpuLevel.lvlData.levelIndices);

    // Update the index buffer to load data to GPU
    registry.patch<VulkanIndexBuffer>(entity);

    // Set the level transform to identity initially
    GW::MATH::GMatrix::IdentityF(gpuLevel.transform);

    // Log the completion
    std::cout << "GPU Level construction complete" << std::endl;
}

//Use this MACRO to connect the ENTT Component Logic
CONNECT_COMPONENT_LOGIC() {
    //register the Window componnent's logic
    registry.on_construct<CPULevel>().connect<Construct_CPULevel>();
    registry.on_construct<GPULevel>().connect<Construct_GPULevel>();
}

} // namespace DRAW