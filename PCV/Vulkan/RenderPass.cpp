#include "RenderPass.h"

#include "Vulkan/VkContext.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Utility.h"
#include "utility/Logger.h"

#include <assert.h>

namespace VulkanAPI
{

RenderPass::RenderPass(VkContext& context) : context(context)
{
}

RenderPass::~RenderPass()
{
    context.device.destroy(renderpass, nullptr);
}

vk::ImageLayout RenderPass::getFinalTransitionLayout(vk::Format format)
{
    vk::ImageLayout result;
    if (VkUtil::isStencil(format) || VkUtil::isDepth(format))
    {
        result = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    }
    else
    {
        result = vk::ImageLayout::eShaderReadOnlyOptimal;
    }
    return result;
}

vk::ImageLayout RenderPass::getAttachmentLayout(vk::Format format)
{
    vk::ImageLayout result;
    if (VkUtil::isStencil(format) || VkUtil::isDepth(format))
    {
        result = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
        result = vk::ImageLayout::eColorAttachmentOptimal;
    }
    return result;
}

void RenderPass::addAttachment(const vk::Format format)
{
    vk::AttachmentDescription attachDescr;
    attachDescr.format = format;
    attachDescr.initialLayout = vk::ImageLayout::eUndefined;
    attachDescr.finalLayout = getFinalTransitionLayout(format);
    attachDescr.samples = vk::SampleCountFlagBits::e1;

    // clear flags
    attachDescr.loadOp = vk::AttachmentLoadOp::eClear;
    attachDescr.storeOp = vk::AttachmentStoreOp::eDontCare;
    attachDescr.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachDescr.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments.emplace_back(attachDescr);
}

void RenderPass::prepare()
{
    assert(!attachments.empty());

    std::vector<vk::AttachmentReference> refs;
    vk::AttachmentReference depth;
    uint32_t attachmentIdx = 0;
    bool hasDepth = false;

    for (const vk::AttachmentDescription& descr : attachments)
    {
        if (VkUtil::isStencil(descr.format) || VkUtil::isDepth(descr.format))
        {
            depth.attachment = attachmentIdx++;
            depth.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            hasDepth = true;
        }
        else
        {
            refs.push_back({attachmentIdx++, vk::ImageLayout::eColorAttachmentOptimal});
        }
    }

    // dependencies for layout transitions
    std::array<vk::SubpassDependency, 2> depends;

    depends[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    depends[0].dstSubpass = 0;
    depends[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    depends[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    depends[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    depends[0].dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    depends[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    depends[1].srcSubpass = 0;
    depends[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    depends[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    depends[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe; 
    depends[1].srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    depends[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
       
    depends[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    // setup the subpass - only using one
    vk::SubpassDescription subpass;
    subpass.colorAttachmentCount = static_cast<uint32_t>(refs.size());
    subpass.pColorAttachments = refs.data();

    if (hasDepth)
    {
        subpass.pDepthStencilAttachment = &depth;
    }

    // and create the pass
    vk::RenderPassCreateInfo createInfo(
        {},
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        1,
        &subpass,
        static_cast<uint32_t>(depends.size()),
        depends.data());

    VK_CHECK_RESULT(context.device.createRenderPass(&createInfo, nullptr, &renderpass));
}

vk::RenderPass& RenderPass::get()
{
    assert(renderpass);
    return renderpass;
}
void RenderPass::setClearColour(OEMaths::colour4& col)
{
    clearCol = col;
}

void RenderPass::setDepthClear(float col)
{
    depthClear = col;
}

bool RenderPass::hasColourAttach()
{
    return !attachments.empty();
}

bool RenderPass::hasDepthAttach()
{
    for (auto& attach : attachments)
    {
        if (VkUtil::isDepth(attach.format))
        {
            return true;
        }
    }
    return false;
}

std::vector<vk::PipelineColorBlendAttachmentState> RenderPass::getColourAttachs()
{
    size_t attachCount = attachments.size();
    assert(attachCount > 0);
    std::vector<vk::PipelineColorBlendAttachmentState> colAttachs(attachCount);

    // for each clear output colour attachment in the renderpass, we need a blend attachment
    for (uint32_t i = 0; i < attachments.size(); ++i)
    {
        vk::PipelineColorBlendAttachmentState colour;
        colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colour.blendEnable = VK_FALSE; //< TODO: need to add blending
        colAttachs.push_back(colour);
    }
    return colAttachs;
}

// ========================= frame buffer =================================
FrameBuffer::FrameBuffer(VkContext& context) : context(context)
{
}

FrameBuffer::~FrameBuffer()
{
    if (fbuffer)
    {
        context.device.destroy(fbuffer);
    }
}

void FrameBuffer::prepare(
    RenderPass& rpass,
    std::vector<ImageView*>& imageViews,
    uint32_t w,
    uint32_t h,
    uint32_t layerCount)
{
    assert(imageViews.size() > 0);

    std::vector<vk::ImageView> views;
    for (auto& view : imageViews)
    {
        views.emplace_back(view->get());
    }

    // store locally the screen extents for use later
    width = w;
    height = h;

    vk::FramebufferCreateInfo frameInfo {{},
                                         rpass.get(),
                                         static_cast<uint32_t>(views.size()),
                                         views.data(),
                                         width,
                                         height,
                                         layerCount};

    VK_CHECK_RESULT(context.device.createFramebuffer(&frameInfo, nullptr, &fbuffer));
}


} // namespace VulkanAPI
