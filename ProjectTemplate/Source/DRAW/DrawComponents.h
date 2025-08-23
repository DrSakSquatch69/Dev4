#ifndef DRAW_COMPONENTS_H
#define DRAW_COMPONENTS_H


#include "./Utility/load_data_oriented.h"

namespace DRAW
{
	//*** TAGS ***//
	struct DoNotRender {}; // Tag to exclude entities from rendering


	//*** COMPONENTS ***//
	struct VulkanRendererInitialization
	{
		std::string vertexShaderName;
		std::string fragmentShaderName;
		VkClearColorValue clearColor;
		VkClearDepthStencilValue depthStencil;
		float fovDegrees;
		float nearPlane;
		float farPlane;
	};

	struct VulkanRenderer
	{
		GW::GRAPHICS::GVulkanSurface vlkSurface;
		VkDevice device = nullptr;
		VkPhysicalDevice physicalDevice = nullptr;
		VkRenderPass renderPass;
		VkShaderModule vertexShader = nullptr;
		VkShaderModule fragmentShader = nullptr;
		VkPipeline pipeline = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		GW::MATH::GMATRIXF projMatrix;
		VkDescriptorSetLayout descriptorLayout = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		std::vector<VkDescriptorSet> descriptorSets;
		VkClearValue clrAndDepth[2];
	};

	struct VulkanVertexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct VulkanIndexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct GeometryData
	{
		unsigned int indexStart, indexCount, vertexStart;
		inline bool operator < (const GeometryData a) const {
			return indexStart < a.indexStart;
		}
	};
	
	struct GPUInstance
	{
		GW::MATH::GMATRIXF	transform;
		H2B::ATTRIBUTES		matData;
	};

	struct VulkanGPUInstanceBuffer
	{
		unsigned long long element_count = 1;
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
	};

	struct SceneData
	{
		GW::MATH::GVECTORF sunDirection, sunColor, sunAmbient, camPos;
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
	};

	struct VulkanUniformBuffer
	{
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
	};


	struct Camera
	{
		GW::MATH::GMATRIXF camMatrix;
	};	

	struct CPULevel
	{
		std::string levelFile;
		std::string modelPath;
		Level_Data lvlData;
	};

	struct GPULevel
	{
		// Store information about the level geometry for rendering
		std::vector<GeometryData> geometryData;

		// Store information about level sections or materials
		std::vector<uint32_t> materialIndices;

		// Store level transformation matrix
		GW::MATH::GMATRIXF transform;

		// Store level bounding information
		struct {
			float minX, minY, minZ;
			float maxX, maxY, maxZ;
		} bounds;
	};

	struct Material
	{
		VkDescriptorSet descriptorSet;
		uint32_t textureIndex;
		struct {
			float r, g, b, a;
		} diffuseColor;
		float specularPower;
		float metallicFactor;
		float roughnessFactor;
	};
	
	struct LevelInstance {}; // Tag for entities that are instances of level objects

	// Also add a Parent component to establish relationships
	struct Parent
	{
		entt::entity parent;
	};
} // namespace DRAW
#endif // !DRAW_COMPONENTS_H
