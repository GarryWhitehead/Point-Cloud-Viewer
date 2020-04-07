#pragma once

#include "Vulkan/Common.h"

#include <vector>

namespace VulkanAPI
{
// forward declearions
class Shader;
struct VkContext;
class RenderPass;

class Pipeline
{
public:
    Pipeline(VkContext& context, RenderPass& rpass, PipelineLayout& layout);
    ~Pipeline();

    vk::PipelineVertexInputStateCreateInfo
    updateVertexInput(std::vector<ShaderProgram::InputBinding>& inputs);
    
    void createLayout(VkContext& context, const std::vector<vk::DescriptorSetLayout>& layouts);
    void addPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size);

    /**
     * Creates a pipeline using render data from the shader program and associates it with the
     * declared renderpass
     */
    void buildPipeline(const RenderPass& renderpass, vk::PolygonMode polygonMode);

    vk::Pipeline& get()
    {
        return pipeline;
    }

private:
    VkContext& context;

    // everything needeed to build the pipeline
    std::vector<vk::VertexInputAttributeDescription> vertexAttrDescr;
    std::vector<vk::VertexInputBindingDescription> vertexBindDescr;

    // dynamic states to be used with this pipeline
    std::vector<vk::DynamicState> dynamicStates;

    // a reference to the renderpass associated with this pipeline
    RenderPass& renderpass;

    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
};

} // namespace VulkanAPI
