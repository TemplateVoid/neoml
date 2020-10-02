/* Copyright © 2017-2020 ABBYY Production LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
--------------------------------------------------------------------------------------------------------------*/

#include <common.h>
#pragma hdrstop

#include <NeoMathEngine/NeoMathEngineDefs.h>

#ifdef NEOML_USE_VULKAN

#include <shaders/common/CommonStruct.h>
#include <VulkanShader.h>
#include <VulkanDll.h>

#include <algorithm>

namespace NeoML {

// The number of threads run for MaliBifrost. We selected this number experimentally for best performance
const int VulkanMaliBifrostThreadCount1D = 32;
const int VulkanMaliBifrostThreadCount2D_X = 4;
const int VulkanMaliBifrostThreadCount2D_Y = 8;
// The number of threads run for Adreno and Regular GPU
const int VulkanAdrenoRegularThreadCount1D = 64;
const int VulkanAdrenoRegularThreadCount2D_X = 8;
const int VulkanAdrenoRegularThreadCount2D_Y = 8;

CVulkanShaderLoader::CVulkanShaderLoader( const CVulkanDevice& vulkanDevice ) :
	device( vulkanDevice ),
	shaders( SH_Count, nullptr)
{}

CVulkanShaderLoader::~CVulkanShaderLoader()
{
	for( size_t i = 0; i < shaders.size(); ++i ) {
		if( shaders[i] == 0 ) {
			continue;
		}

		if( shaders[i]->Module != VK_NULL_HANDLE ) {
			device.vkDestroyShaderModule( shaders[i]->Module, 0 );
		}
		if( shaders[i]->Layout != VK_NULL_HANDLE ) {
			device.vkDestroyPipelineLayout( shaders[i]->Layout, 0 );
		}
		if( shaders[i]->DescLayout != VK_NULL_HANDLE ) {
			device.vkDestroyDescriptorSetLayout( shaders[i]->DescLayout, 0 );
		}

		for (auto& pair : shaders[i]->pipelines) {
			device.vkDestroyPipeline( pair.second, nullptr );
		}

		delete shaders[i];
	}
}

CVulkanShader CVulkanShaderLoader::GetShaderData(TShader id, bool isIB, const uint32_t* code, int codeLen,
	size_t paramSize, int imageCount, int samplerCount, int bufferCount, int dimensions,
	int gridWidth, int gridHeight, int gridDepth)
{
	assert(imageCount <= IMAGE_MAX_COUNT);
	assert(samplerCount <= SAMPLER_MAX_COUNT);
	assert(dimensions >= 1);
	assert(dimensions <= 3);

	if (shaders[id] == 0) {
		// Create the shader data
		shaders[id] = new CVulkanShaderData();

		shaders[id]->IsImageBased = isIB && device.IsImageBased;

		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.pCode = code;
		shaderInfo.codeSize = codeLen;

		vkSucceded(device.vkCreateShaderModule(&shaderInfo, 0, &shaders[id]->Module));

		std::vector< VkDescriptorSetLayoutBinding, CrtAllocator<VkDescriptorSetLayoutBinding> > bindingInfo;

		bindingInfo.reserve(2 * VulkanMaxBindingCount + 1);
		int curBinding = 1;
		for (int i = 0; i < bufferCount; ++i) {
			VkDescriptorSetLayoutBinding info = {};
			info.binding = curBinding++;
			info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			info.descriptorCount = 1;
			info.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindingInfo.push_back(info);
		}
		for (int i = 0; i < imageCount; ++i) {
			VkDescriptorSetLayoutBinding info = {};
			info.binding = IMAGE_BINDING_NUM(i);
			info.descriptorType =
				shaders[id]->IsImageBased ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			info.descriptorCount = 1;
			info.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindingInfo.push_back(info);
		}
		for (int i = 0; i < samplerCount; ++i) {
			VkDescriptorSetLayoutBinding info = {};
			info.binding = SAMPLER_BINDING_NUM(i);
			info.descriptorType =
				shaders[id]->IsImageBased ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			info.descriptorCount = 1;
			info.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindingInfo.push_back(info);
		}
		VkDescriptorSetLayoutCreateInfo descInfo = {};
		descInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descInfo.bindingCount = static_cast<int>(bindingInfo.size());
		descInfo.pBindings = bindingInfo.data();
		vkSucceded(device.vkCreateDescriptorSetLayout(&descInfo, 0, &shaders[id]->DescLayout));

		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &shaders[id]->DescLayout;
		VkPushConstantRange pushConstantRange;
		if (paramSize > 0 || samplerCount > 0 || imageCount > 0) {
			size_t pushConstantSize = PUSH_CONSTANT_PARAM_OFFSET + paramSize;
			assert(pushConstantSize <= device.Properties.limits.maxPushConstantsSize);
			pushConstantRange = { VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(pushConstantSize) };
			layoutInfo.pushConstantRangeCount = 1;
			layoutInfo.pPushConstantRanges = &pushConstantRange;
		}

		vkSucceded(device.vkCreatePipelineLayout(&layoutInfo, 0, &shaders[id]->Layout));
	}

	CWorkGroup workGroup;
	calculateThreadGroupSize(dimensions, workGroup.X, workGroup.Y, workGroup.Z, gridWidth, gridHeight, gridDepth);

	auto& pipelines = shaders[id]->pipelines;
	if (pipelines.find(workGroup) == pipelines.end()) {

		std::vector<VkSpecializationMapEntry> specializationMapEntries(3);
		specializationMapEntries[0].constantID = 0;
		specializationMapEntries[0].offset = 0 * sizeof(int);
		specializationMapEntries[0].size = sizeof(int);

		specializationMapEntries[1].constantID = 1;
		specializationMapEntries[1].offset = 1 * sizeof(int);
		specializationMapEntries[1].size = sizeof(int);

		specializationMapEntries[2].constantID = 2;
		specializationMapEntries[2].offset = 2 * sizeof(int);
		specializationMapEntries[2].size = sizeof(int);

		int specializationData[3] = { workGroup.X, workGroup.Y, workGroup.Z };

		VkSpecializationInfo specializationInfo;
		specializationInfo.mapEntryCount = 3;
		specializationInfo.pMapEntries = &specializationMapEntries[0];
		specializationInfo.dataSize = 3 * sizeof(int);
		specializationInfo.pData = specializationData;

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.pNext = 0;
		pipelineShaderStageCreateInfo.flags = 0;
		pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineShaderStageCreateInfo.module = shaders[id]->Module;
		pipelineShaderStageCreateInfo.pName = "main";
		pipelineShaderStageCreateInfo.pSpecializationInfo = &specializationInfo;

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = pipelineShaderStageCreateInfo;
		pipelineInfo.layout = shaders[id]->Layout;

		vkSucceded(device.vkCreateComputePipelines(VK_NULL_HANDLE, 1, &pipelineInfo, 0, &pipelines[workGroup]));
	}

	CVulkanShader result(*shaders[id]);
	result.GroupSizeX = workGroup.X;
	result.GroupSizeY = workGroup.Y;
	result.GroupSizeZ = workGroup.Z;
	result.Pipeline = pipelines[workGroup];
	return result;
}

int calculate(int gridWidth, int gridHeight, uint16_t& width, uint16_t& height, int size = CWorkGroup::Size)
{
	int result = (std::numeric_limits<int>::max)();
	for (int h = 1, w = size; h <= size; h = h << 1, w = w >> 1) {
		const int count = Ceil(gridWidth, w) * Ceil(gridHeight, h);
		if (count < result || ((count == result) && std::abs(w - h) < std::abs(width - height))) {
			result = count;
			width = w;
			height = h;
		}
	}
	return result;
}

int calculate(int gridWidth, int gridHeight, int gridDepth, uint16_t& width, uint16_t& height, uint16_t& depth)
{
	int result = (std::numeric_limits<int>::max)();
	constexpr int maxDepth = 32;
	for (int d = 1; d <= gridDepth && d < maxDepth; d = d << 1) {
		uint16_t w;
		uint16_t h;
		const int count = Ceil(gridDepth, d) * calculate(gridWidth, gridHeight, w, h, CWorkGroup::Size/d);
		if (count < result) {
			result = count;
			width = w;
			height = h;
			depth = d;
		}
	}
	return result;
}

void CVulkanShaderLoader::calculateThreadGroupSize( int dimensions, 
	uint16_t& threadGroupSizeX, uint16_t& threadGroupSizeY, uint16_t& threadGroupSizeZ, 
	int gridWidth, int gridHeight, int gridDepth ) const
{
	assert( dimensions >= 1 );
	assert( dimensions <= 3 );

	switch( dimensions ) {
		case 1: {
			threadGroupSizeX = CWorkGroup::Size;
				//( device.Type == VDT_MaliBifrost ) ? VulkanMaliBifrostThreadCount1D : VulkanAdrenoRegularThreadCount1D;
			threadGroupSizeY = 1;
			threadGroupSizeZ = 1;
			break;
		}
		case 2:
			calculate(gridWidth, gridHeight, threadGroupSizeX, threadGroupSizeY);
			//ASSERT_EXPR(threadGroupSizeX * threadGroupSizeY == CWorkGroup::Size);
			//threadGroupSizeX = 8;
			//threadGroupSizeY = 8;
			//threadGroupSizeZ = 1;
			break;
		case 3: {
			calculate(gridWidth, gridHeight, gridDepth, threadGroupSizeX, threadGroupSizeY, threadGroupSizeZ);
			//ASSERT_EXPR(threadGroupSizeX * threadGroupSizeY * threadGroupSizeZ == CWorkGroup::Size);
			//threadGroupSizeX = 8;
			//threadGroupSizeY = 8;
			//threadGroupSizeZ = 1;
			break;
		}
	}
}


} // namespace NeoML

#endif // NEOML_USE_VULKAN
