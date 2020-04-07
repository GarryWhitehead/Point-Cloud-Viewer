#include "Pipeline.h"

#include "Vulkan/RenderPass.h"
#include "Vulkan/VkContext.h"

namespace VulkanAPI
{

Pipeline::Pipeline(VkContext& context, RenderPass& rpass, PipelineLayout& layout)
    : context(context), renderpass(rpass), pipelineLayout(layout)
{
}

Pipeline::~Pipeline()
{
}

void Pipeline::addPushConstant(vk::ShaderStageFlags flags, uint32_t offset, uint32_t size)
{
    vk::PushConstantRange push(flags, offset, size);
    pConstants.push_back(push);
}

void Pipeline::createLayout(VkContext& context, const std::vector<vk::DescriptorSetLayout>& layouts)
{
    vk::PipelineLayoutCreateInfo pipelineInfo(
        {},
        static_cast<uint32_t>(layouts.size()),
        layouts.data(),
        static_cast<uint32_t>(pConstants.size()),
        pConstants.data());

    VK_CHECK_RESULT(context.device.createPipelineLayout(&pipelineInfo, nullptr, &layout));
}

vk::PipelineVertexInputStateCreateInfo
Pipeline::updateVertexInput(std::vector<ShaderProgram::InputBinding>& inputs)
{
    vk::PipelineVertexInputStateCreateInfo vertexInputState;

    // check for empty vertex input
    if (inputs.empty())
    {
        vertexInputState.vertexAttributeDescriptionCount = 0;
        vertexInputState.pVertexAttributeDescriptions = nullptr;
        vertexInputState.vertexBindingDescriptionCount = 0;
        vertexInputState.pVertexBindingDescriptions = nullptr;
        return vertexInputState;
    }

    for (const ShaderProgram::InputBinding& input : inputs)
    {
        vertexAttrDescr.push_back({input.loc, 0, input.format, input.stride});
    }

    // first sort the attributes so they are in order of location
    std::sort(
        vertexAttrDescr.begin(),
        vertexAttrDescr.end(),
        [](const vk::VertexInputAttributeDescription lhs,
           const vk::VertexInputAttributeDescription rhs) { return lhs.location < rhs.location; });

    vertexInputState.vertexAttributeDescriptionCount = vertexAttrDescr.size();
    vertexInputState.pVertexAttributeDescriptions = vertexAttrDescr.data();
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindDescr.size());
    vertexInputState.pVertexBindingDescriptions = vertexBindDescr.data();

    return vertexInputState;
}

void Pipeline::buildPipeline(const RenderPass& renderpass, vk::PolygonMode polygonMode)
{
    auto& renderState = program.renderState;

    // calculate the offset and stride size
    vk::PipelineVertexInputStateCreateInfo vertInputState = updateVertexInput(program.inputs);

    // ============== primitive topology =====================
    vk::PipelineInputAssemblyStateCreateInfo assemblyState;
    assemblyState.topology = renderState->rastState.topology;
    assemblyState.primitiveRestartEnable = renderState->rastState.primRestart;

    // ============== multi-sample state =====================
    vk::PipelineMultisampleStateCreateInfo sampleState;


    // ============== depth/stenicl state ====================
    vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    depthStencilState.depthTestEnable =  VK_TRUE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

    // ============== stencil state =====================
    depthStencilState.stencilTestEnable = VK_FALSE;

    // ============ raster state =======================
    vk::PipelineRasterizationStateCreateInfo rasterState;
    rasterState.cullMode = vk::CullModeFlagBits::eBack;
    rasterState.frontFace = vk::FrontFace::eCounterClockwise;
    rasterState.polygonMode = polygonMode;

    // ============ dynamic states ====================
    vk::PipelineDynamicStateCreateInfo dynamicCreateState;
    dynamicCreateState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicCreateState.pDynamicStates = dynamicStates.data();

    // =============== viewport state ====================
    vk::PipelineViewportStateCreateInfo viewportState;
    vk::Viewport viewPort(
        0.0f,
        0.0f,
        static_cast<float>(renderpass.getWidth()),
        static_cast<float>(renderpass.getHeight()),
        0.0f,
        1.0f);
    vk::Rect2D scissor(
        vk::Offset2D(0, 0), vk::Extent2D((uint32_t) viewPort.width, (uint32_t) viewPort.height));
    viewportState.pViewports = &viewPort;
    viewportState.viewportCount = 1;
    viewportState.pScissors = &scissor;
    viewportState.scissorCount = 1;

    // ============= colour attachment =================
    auto colAttachments = renderpass.getColourAttachs();
    vk::PipelineColorBlendStateCreateInfo colourBlendState;
    colourBlendState.attachmentCount = static_cast<uint32_t>(colAttachments.size());
    colourBlendState.pAttachments = colAttachments.data();

    std::vector<vk::PipelineShaderStageCreateInfo> shaderData;
    for (auto& stage : program.stages)
    {
        shaderData.emplace_back(stage.getShader()->get());
    }

    // ================= create the pipeline =======================
    vk::GraphicsPipelineCreateInfo createInfo(
        {},
        static_cast<uint32_t>(shaderData.size()),
        shaderData.data(),
        &vertInputState,
        &assemblyState,
        nullptr,
        &viewportState,
        &rasterState,
        &sampleState,
        &depthStencilState,
        &colourBlendState,
        &dynamicCreateState,
        pipelineLayout,
        renderpass.get(),
        0,
        nullptr,
        0);

    VK_CHECK_RESULT(context.device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
}

} // namespace VulkanAPI
