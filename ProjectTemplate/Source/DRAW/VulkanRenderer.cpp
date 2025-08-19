#include "DrawComponents.h"
#include "../CCL.h"
// component dependencies
#include "./Utility/FileIntoString.h"

#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

namespace DRAW
{
	//*** HELPER METHODS ***//

	VkViewport CreateViewportFromWindowDimensions(unsigned int windowWidth, unsigned int windowHeight)
	{
		VkViewport retval = {};
		retval.x = 0;
		retval.y = 0;
		retval.width = static_cast<float>(windowWidth);
		retval.height = static_cast<float>(windowHeight);
		retval.minDepth = 0;
		retval.maxDepth = 1;
		return retval;
	}

	VkRect2D CreateScissorFromWindowDimensions(unsigned int windowWidth, unsigned int windowHeight)
	{
		VkRect2D retval = {};
		retval.offset.x = 0;
		retval.offset.y = 0;
		retval.extent.width = windowWidth;
		retval.extent.height = windowHeight;
		return retval;
	}

	void InitializeDescriptors(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);

		unsigned int frameCount;
		vulkanRenderer.vlkSurface.GetSwapchainImageCount(frameCount);
		vulkanRenderer.descriptorSets.resize(frameCount);

#pragma region Descriptor Layout
		VkDescriptorSetLayoutBinding layoutBinding[2] = {};
		layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding[0].descriptorCount = 1;
		layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding[0].binding = 0;
		layoutBinding[0].pImmutableSamplers = nullptr;
		
		layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBinding[1].descriptorCount = 1;
		layoutBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding[1].binding = 1;
		layoutBinding[1].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo setCreateInfo = {};
		setCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setCreateInfo.bindingCount = 2;
		setCreateInfo.pBindings = layoutBinding;
		setCreateInfo.flags = 0;
		setCreateInfo.pNext = nullptr;
		vkCreateDescriptorSetLayout(vulkanRenderer.device, &setCreateInfo, nullptr, &vulkanRenderer.descriptorLayout);
#pragma endregion

#pragma region Descriptor Pool
		VkDescriptorPoolCreateInfo descriptorpool_create_info = {};
		descriptorpool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkDescriptorPoolSize descriptorpool_size[2] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frameCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frameCount }
		};
		descriptorpool_create_info.poolSizeCount = 2;
		descriptorpool_create_info.pPoolSizes = descriptorpool_size;
		descriptorpool_create_info.maxSets = frameCount;
		descriptorpool_create_info.flags = 0;
		descriptorpool_create_info.pNext = nullptr;
		vkCreateDescriptorPool(vulkanRenderer.device, &descriptorpool_create_info, nullptr, &vulkanRenderer.descriptorPool);
#pragma endregion

#pragma region Allocate Descriptor Sets
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.descriptorPool = vulkanRenderer.descriptorPool;
		allocateInfo.pSetLayouts = &vulkanRenderer.descriptorLayout;
		allocateInfo.pNext = nullptr;
		for (int i = 0; i < frameCount; i++)
		{
			vkAllocateDescriptorSets(vulkanRenderer.device, &allocateInfo, &vulkanRenderer.descriptorSets[i]);
		}
#pragma endregion

		// Add the 2 buffers, this will create the initial buffers so we can finish building our descriptor set
		auto& storageBuffer = registry.emplace<VulkanGPUInstanceBuffer>(entity,
			VulkanGPUInstanceBuffer{16}); // Start with a reasonable size of elements. The Buffer will grow if it needs to later
		auto& uniformBuffer = registry.emplace<VulkanUniformBuffer>(entity);

		 
		for (int i = 0; i < frameCount; i++)
		{
			
			VkDescriptorBufferInfo uniformBufferInfo = { uniformBuffer.buffer[i], 0, VK_WHOLE_SIZE};
			VkWriteDescriptorSet uniformWrite = {};
			uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			uniformWrite.dstSet = vulkanRenderer.descriptorSets[i];
			uniformWrite.dstBinding = 0; // 0 For the uniform buffer
			uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformWrite.descriptorCount = 1;
			uniformWrite.pBufferInfo = &uniformBufferInfo;

			VkDescriptorBufferInfo storageBufferInfo = { storageBuffer.buffer[i], 0, VK_WHOLE_SIZE };
			VkWriteDescriptorSet storageWrite = {};
			storageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			storageWrite.dstSet = vulkanRenderer.descriptorSets[i];
			storageWrite.dstBinding = 1; // 1 For the storage buffer
			storageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			storageWrite.descriptorCount = 1;
			storageWrite.pBufferInfo = &storageBufferInfo;

			VkWriteDescriptorSet descriptorWrites[] = { uniformWrite, storageWrite };
			vkUpdateDescriptorSets(vulkanRenderer.device, 2, descriptorWrites, 0, nullptr);
		}

	}

	void InitializeGraphicsPipeline(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);
		GW::SYSTEM::GWindow win = registry.get<GW::SYSTEM::GWindow>(entity);

		// Create Pipeline & Layout (Thanks Tiny!)
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vulkanRenderer.vertexShader;
		stage_create_info[0].pName = "main";

		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = vulkanRenderer.fragmentShader;
		stage_create_info[1].pName = "main";

		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;

		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(H2B::VERTEX);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertex_attribute_description[3];
		vertex_attribute_description[0].binding = 0;
		vertex_attribute_description[0].location = 0;
		vertex_attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[0].offset = offsetof(H2B::VERTEX, pos);

		vertex_attribute_description[1].binding = 0;
		vertex_attribute_description[1].location = 1;
		vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[1].offset = offsetof(H2B::VERTEX, uvw);

		vertex_attribute_description[2].binding = 0;
		vertex_attribute_description[2].location = 2;
		vertex_attribute_description[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[2].offset = offsetof(H2B::VERTEX, nrm);

		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;

		unsigned int windowWidth, windowHeight;
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
		VkViewport viewport = CreateViewportFromWindowDimensions(windowWidth, windowHeight);

		VkRect2D scissor = CreateScissorFromWindowDimensions(windowWidth, windowHeight);

		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;

		// Dynamic State 
		VkDynamicState dynamic_states[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_states;

		InitializeDescriptors(registry, entity);

		VkPushConstantRange pushConstantRanges[2] = {};

		// Material data push constant range (for fragment shader)
		pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRanges[0].offset = 0;
		pushConstantRanges[0].size = sizeof(H2B::ATTRIBUTES);

		// Transform matrix push constant range (for vertex shader)
		pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRanges[1].offset = sizeof(H2B::ATTRIBUTES);
		pushConstantRanges[1].size = sizeof(GW::MATH::GMATRIXF);

		// Update the pipeline layout create info
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &vulkanRenderer.descriptorLayout;
		pipeline_layout_create_info.pushConstantRangeCount = 2;
		pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

		// Create the pipeline layout
		vkCreatePipelineLayout(vulkanRenderer.device, &pipeline_layout_create_info, nullptr, &vulkanRenderer.pipelineLayout);
		
		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = vulkanRenderer.pipelineLayout;
		pipeline_create_info.renderPass = vulkanRenderer.renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(vulkanRenderer.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &vulkanRenderer.pipeline);
	}

	

	//*** SYSTEMS ***//

	// run this code when a VulkanRenderer component is connected
	void Construct_VulkanRenderer(entt::registry& registry, entt::entity entity)
	{
		if (!registry.all_of<GW::SYSTEM::GWindow>(entity))
		{
			std::cout << "Window not added to the registry yet!" << std::endl;
			abort();
			return;
		}

		if (!registry.all_of<VulkanRendererInitialization>(entity))
		{
			std::cout << "Initialization Data not added to the registry yet!" << std::endl;
			abort();
			return;
		}

		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);
		auto& initializationData = registry.get<VulkanRendererInitialization>(entity);

		GW::SYSTEM::GWindow win = registry.get<GW::SYSTEM::GWindow>(entity);		
#ifndef NDEBUG
		const char* debugLayers[] = {
			"VK_LAYER_KHRONOS_validation", // standard validation layer
		};
		if (-vulkanRenderer.vlkSurface.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
			sizeof(debugLayers) / sizeof(debugLayers[0]),
			debugLayers, 0, nullptr, 0, nullptr, false))
#else
		if (-vulkanRenderer.vlkSurface.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
#endif
		{
			std::cout << "Failed to create Vulkan Surface!" << std::endl;
			abort();
			return;
		}

		vulkanRenderer.clrAndDepth[0].color = initializationData.clearColor;
		vulkanRenderer.clrAndDepth[1].depthStencil = initializationData.depthStencil;

		// Create Projection matrix
		float aspectRatio;
		vulkanRenderer.vlkSurface.GetAspectRatio(aspectRatio);
		GW::MATH::GMatrix::ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN_F(initializationData.fovDegrees), aspectRatio, initializationData.nearPlane, initializationData.farPlane, vulkanRenderer.projMatrix);

		
		vulkanRenderer.vlkSurface.GetDevice((void**)&vulkanRenderer.device);
		vulkanRenderer.vlkSurface.GetPhysicalDevice((void**)&vulkanRenderer.physicalDevice);
		vulkanRenderer.vlkSurface.GetRenderPass((void**)&vulkanRenderer.renderPass);

		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif

		// Vertex Shader
		std::string vertexShaderSource = ReadFileIntoString(initializationData.vertexShaderName.c_str());

		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			std::cout << "Vertex Shader Errors : \n" << shaderc_result_get_error_message(result) << std::endl;
			abort();
			return;
		}

		GvkHelper::create_shader_module(vulkanRenderer.device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vulkanRenderer.vertexShader);

		shaderc_result_release(result); // done

		// Fragment Shader
		std::string fragmentShaderSource = ReadFileIntoString(initializationData.fragmentShaderName.c_str());

		result = shaderc_compile_into_spv( // compile
			compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			std::cout << "Fragment Shader Errors : \n" << shaderc_result_get_error_message(result) << std::endl;
			abort();
			return;
		}

		GvkHelper::create_shader_module(vulkanRenderer.device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vulkanRenderer.fragmentShader);

		shaderc_result_release(result); // done

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		InitializeGraphicsPipeline(registry, entity);

		// Remove the initializtion data as we no longer need it
		registry.remove<VulkanRendererInitialization>(entity);

	}

	void Render_Level(entt::registry& registry, entt::entity entity, VkCommandBuffer commandBuffer)
	{
		// Check if the entity has the necessary components for level rendering
		if (!registry.all_of<GPULevel, VulkanVertexBuffer, VulkanIndexBuffer>(entity))
			return;

		// Get the vertex and index buffers
		auto& vertexBuffer = registry.get<VulkanVertexBuffer>(entity);
		auto& indexBuffer = registry.get<VulkanIndexBuffer>(entity);

		// Make sure the buffers are valid
		if (vertexBuffer.buffer == VK_NULL_HANDLE || indexBuffer.buffer == VK_NULL_HANDLE)
			return;

		// Bind the vertex and index buffers
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Get the index count from the level data
		if (registry.all_of<CPULevel>(entity)) {
			auto& cpuLevel = registry.get<CPULevel>(entity);

			// Draw the entire level with a single draw call
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(cpuLevel.lvlData.levelIndices.size()),
				1, 0, 0, 0);

			// Log the rendering
			std::cout << "Rendering level with "
				<< cpuLevel.lvlData.levelIndices.size() << " indices and "
				<< cpuLevel.lvlData.levelVertices.size() << " vertices" << std::endl;
		}
	}

	void Render_LevelWithMaterials(entt::registry& registry, entt::entity entity, VkCommandBuffer commandBuffer)
	{
		// Check if the entity has the necessary components for level rendering
		if (!registry.all_of<GPULevel, VulkanVertexBuffer, VulkanIndexBuffer>(entity))
			return;

		// Get the vertex and index buffers
		auto& vertexBuffer = registry.get<VulkanVertexBuffer>(entity);
		auto& indexBuffer = registry.get<VulkanIndexBuffer>(entity);
		auto& gpuLevel = registry.get<GPULevel>(entity);

		// Make sure the buffers are valid
		if (vertexBuffer.buffer == VK_NULL_HANDLE || indexBuffer.buffer == VK_NULL_HANDLE)
			return;

		// Bind the vertex and index buffers
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// If we have geometry data in the GPULevel, use it for rendering
		if (!gpuLevel.geometryData.empty()) {
			// Render each geometry section with its material
			for (size_t i = 0; i < gpuLevel.geometryData.size(); i++) {
				const auto& geom = gpuLevel.geometryData[i];

				// If we have material indices, bind the appropriate material
				if (i < gpuLevel.materialIndices.size()) {
					uint32_t materialIndex = gpuLevel.materialIndices[i];
					// TODO: Bind material-specific data or descriptor sets here
				}

				// Draw this section of geometry
				vkCmdDrawIndexed(commandBuffer,
					geom.indexCount,
					1,
					geom.indexStart,
					geom.vertexStart,
					0);
			}

			// Log the rendering
			std::cout << "Rendering level with " << gpuLevel.geometryData.size()
				<< " geometry sections" << std::endl;
		}
		// Fall back to the CPULevel data if GPULevel doesn't have geometry data
		else if (registry.all_of<CPULevel>(entity)) {
			auto& cpuLevel = registry.get<CPULevel>(entity);

			// Draw the entire level with a single draw call
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(cpuLevel.lvlData.levelIndices.size()),
				1, 0, 0, 0);

			// Log the rendering
			std::cout << "Rendering level with "
				<< cpuLevel.lvlData.levelIndices.size() << " indices and "
				<< cpuLevel.lvlData.levelVertices.size() << " vertices" << std::endl;
		}
	}
	
	void Render_LevelInstances(entt::registry& registry, entt::entity entity, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame)
	{
		// Check if we have the necessary components
		if (!registry.all_of<GPULevel, VulkanVertexBuffer, VulkanIndexBuffer>(entity))
			return;

		// Get the level components
		auto& gpuLevel = registry.get<GPULevel>(entity);
		auto& vertexBuffer = registry.get<VulkanVertexBuffer>(entity);
		auto& indexBuffer = registry.get<VulkanIndexBuffer>(entity);

		// Make sure the buffers are valid
		if (vertexBuffer.buffer == VK_NULL_HANDLE || indexBuffer.buffer == VK_NULL_HANDLE)
			return;

		// Bind the vertex and index buffers
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Create a view of all entities with GeometryData and GPUInstance components
		// that are associated with this level
		auto levelInstancesView = registry.view<GeometryData, GPUInstance>(entt::exclude<DoNotRender>);

		// Sort instances by material to minimize state changes
		std::vector<entt::entity> sortedInstances;
		for (auto instanceEntity : levelInstancesView)
			sortedInstances.push_back(instanceEntity);

		std::sort(sortedInstances.begin(), sortedInstances.end(),
			[&registry](const entt::entity& a, const entt::entity& b) {
				const auto& instA = registry.get<GPUInstance>(a);
				const auto& instB = registry.get<GPUInstance>(b);

				// Sort by material attributes (e.g., diffuse color)
				return memcmp(&instA.matData, &instB.matData, sizeof(H2B::ATTRIBUTES)) < 0;
			});

		// Track the current material to avoid redundant bindings
		H2B::ATTRIBUTES currentMaterial = {};
		bool firstMaterial = true;

		// Render all instances
		for (auto instanceEntity : sortedInstances)
		{
			const auto& geom = registry.get<GeometryData>(instanceEntity);
			const auto& inst = registry.get<GPUInstance>(instanceEntity);

			// If the material changed, update the push constants
			if (firstMaterial || memcmp(&currentMaterial, &inst.matData, sizeof(H2B::ATTRIBUTES)) != 0)
			{
				// Update the current material
				currentMaterial = inst.matData;
				firstMaterial = false;

				// Push the material data to the shader
				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					0,  // offset
					sizeof(H2B::ATTRIBUTES),
					&inst.matData
				);
			}

			// Push the instance transform to the shader
			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				sizeof(H2B::ATTRIBUTES),  // offset after material data
				sizeof(GW::MATH::GMATRIXF),
				&inst.transform
			);

			// Draw this instance
			vkCmdDrawIndexed(
				commandBuffer,
				geom.indexCount,
				1,  // instance count
				geom.indexStart,
				geom.vertexStart,
				0   // first instance
			);
		}

		// Log the rendering
		std::cout << "Rendered " << sortedInstances.size() << " level instances" << std::endl;
	}

	void BindMaterial(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Material& material)
	{
		// Bind the material's descriptor set
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);

		// Push material constants if needed
		struct MaterialConstants {
			float diffuseColor[4];
			float specularPower;
			float metallicFactor;
			float roughnessFactor;
			float padding;
		} constants;

		constants.diffuseColor[0] = material.diffuseColor.r;
		constants.diffuseColor[1] = material.diffuseColor.g;
		constants.diffuseColor[2] = material.diffuseColor.b;
		constants.diffuseColor[3] = material.diffuseColor.a;
		constants.specularPower = material.specularPower;
		constants.metallicFactor = material.metallicFactor;
		constants.roughnessFactor = material.roughnessFactor;

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(MaterialConstants), &constants);
	}

	// run this code when a VulkanRenderer component is updated
	void Update_VulkanRenderer(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);

		if (-vulkanRenderer.vlkSurface.StartFrame(2, vulkanRenderer.clrAndDepth))
		{
			std::cout << "Failed to start frame!" << std::endl;
			return;
		}

		auto win = registry.get<GW::SYSTEM::GWindow>(entity);
		unsigned int frame;
		vulkanRenderer.vlkSurface.GetSwapchainCurrentImage(frame);

		VkCommandBuffer commandBuffer;
		unsigned int currentBuffer;
		vulkanRenderer.vlkSurface.GetSwapchainCurrentImage(currentBuffer);
		vulkanRenderer.vlkSurface.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);

		unsigned int windowWidth, windowHeight;
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
		VkViewport viewport = CreateViewportFromWindowDimensions(windowWidth, windowHeight);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = CreateScissorFromWindowDimensions(windowWidth, windowHeight);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanRenderer.pipeline);

		// Update uniform and storage buffers
		registry.patch<VulkanUniformBuffer>(entity);

		// Check for presence of the buffers first as they take a few frames before they are created
		if (registry.all_of< VulkanVertexBuffer, VulkanIndexBuffer>(entity))
		{
			auto& vertexBuffer = registry.get<VulkanVertexBuffer>(entity);
			auto& indexBuffer = registry.get<VulkanIndexBuffer>(entity);

			if (vertexBuffer.buffer != VK_NULL_HANDLE && indexBuffer.buffer != VK_NULL_HANDLE)
			{
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
			}

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanRenderer.pipelineLayout, 0, 1, &vulkanRenderer.descriptorSets[frame], 0, nullptr);

			// Create a group for all entities with GeometryData and GPUInstance
			auto renderGroup = registry.group<GeometryData, GPUInstance, entt::exclude_t<DRAW::DoNotRender>>();

			// Sort the group by GeometryData (by indexStart)
			renderGroup.sort<GeometryData>([](const GeometryData& a, const GeometryData& b) {
				return a < b;
				});

			// *** START OF STEP 3C IMPLEMENTATION ***
			// STEP 3C: Create containers for instance data
			std::vector<GPUInstance> instances;
			std::map<GeometryData, size_t> geometryMap;

			// Loop through all entities with GeometryData and GPUInstance
			for (auto entityId : renderGroup)
			{
				auto& geom = renderGroup.get<GeometryData>(entityId);
				auto& inst = renderGroup.get<GPUInstance>(entityId);

				// Add this instance to our vector
				instances.push_back(inst);

				// Update the map entry for this geometry
				if (geometryMap.find(geom) == geometryMap.end()) {
					// First instance of this geometry
					geometryMap[geom] = 1;
				}
				else {
					// Increment instance count for this geometry
					geometryMap[geom]++;
				}
			}

			// If we have instances, update the GPU buffer
			if (!instances.empty())
			{
				// Add the instances vector to the entity for the VulkanGPUInstanceBuffer to use
				registry.emplace_or_replace<std::vector<GPUInstance>>(entity, std::move(instances));

				// Update the GPU buffer with the new instances
				registry.patch<VulkanGPUInstanceBuffer>(entity);
			}
			// *** END OF STEP 3C IMPLEMENTATION ***

			// *** START OF STEP 3D IMPLEMENTATION ***
			// STEP 3D: Use the structured buffer and the list of GeometryData to draw the scene
			if (registry.all_of<VulkanGPUInstanceBuffer>(entity))
			{
				auto& gpuInstanceBuffer = registry.get<VulkanGPUInstanceBuffer>(entity);

				// Track the current geometry to avoid redundant draw calls
				GeometryData currentGeometry = {};
				size_t instanceCount = 0;
				size_t firstInstance = 0;
				bool firstGeometry = true;

				// Process each unique geometry and draw all its instances at once
				for (const auto& [geom, count] : geometryMap)
				{
					// Draw this geometry with all its instances
					vkCmdDrawIndexed(
						commandBuffer,
						geom.indexCount,
						count,  // Instance count from our map
						geom.indexStart,
						geom.vertexStart,
						firstInstance  // First instance in the buffer
					);

					// Update the first instance for the next geometry
					firstInstance += count;
				}
			}
			else
			{
				// Fallback to individual draw calls if the GPU instance buffer isn't available
				// Track the current material to avoid redundant bindings
				H2B::ATTRIBUTES currentMaterial = {};
				bool firstMaterial = true;

				// Iterate and issue draw calls
				for (auto entityId : renderGroup) {
					auto& geom = renderGroup.get<GeometryData>(entityId);
					auto& inst = renderGroup.get<GPUInstance>(entityId);

					// If the material changed, update the push constants
					if (firstMaterial || memcmp(&currentMaterial, &inst.matData, sizeof(H2B::ATTRIBUTES)) != 0)
					{
						// Update the current material
						currentMaterial = inst.matData;
						firstMaterial = false;

						// Push the material data to the shader
						vkCmdPushConstants(
							commandBuffer,
							vulkanRenderer.pipelineLayout,
							VK_SHADER_STAGE_FRAGMENT_BIT,
							0,  // offset
							sizeof(H2B::ATTRIBUTES),
							&inst.matData
						);
					}

					// Push the instance transform to the shader
					vkCmdPushConstants(
						commandBuffer,
						vulkanRenderer.pipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT,
						sizeof(H2B::ATTRIBUTES),  // offset after material data
						sizeof(GW::MATH::GMATRIXF),
						&inst.transform
					);

					// Issue the draw call
					vkCmdDrawIndexed(
						commandBuffer,
						geom.indexCount,
						1,  // instance count
						geom.indexStart,
						geom.vertexStart,
						0   // first instance
					);
				}
			}
			
		}

		vulkanRenderer.vlkSurface.EndFrame(true);
	}

	// run this code when a VulkanRenderer component is updated
	void Destroy_VulkanRenderer(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);
		// wait till everything has completed
		vkDeviceWaitIdle(vulkanRenderer.device);
		// Remove Buffer compontents
		registry.remove<VulkanIndexBuffer>(entity);
		registry.remove<VulkanVertexBuffer>(entity);
		registry.remove<VulkanGPUInstanceBuffer>(entity);
		registry.remove<VulkanUniformBuffer>(entity);


		vkDestroyDescriptorSetLayout(vulkanRenderer.device, vulkanRenderer.descriptorLayout, nullptr);
		vkDestroyDescriptorPool(vulkanRenderer.device, vulkanRenderer.descriptorPool, nullptr);

		// Release allocated shaders & pipeline
		vkDestroyShaderModule(vulkanRenderer.device, vulkanRenderer.vertexShader, nullptr);
		vkDestroyShaderModule(vulkanRenderer.device, vulkanRenderer.fragmentShader, nullptr);
		vkDestroyPipelineLayout(vulkanRenderer.device, vulkanRenderer.pipelineLayout, nullptr);
		vkDestroyPipeline(vulkanRenderer.device, vulkanRenderer.pipeline, nullptr);
	}

	// Use this MACRO to connect the EnTT Component Logic
	CONNECT_COMPONENT_LOGIC() {
		// register the Window component's logic
		registry.on_construct<VulkanRenderer>().connect<Construct_VulkanRenderer>();
		registry.on_update<VulkanRenderer>().connect<Update_VulkanRenderer>();
		registry.on_destroy<VulkanRenderer>().connect<Destroy_VulkanRenderer>();
	}

} // namespace DRAW