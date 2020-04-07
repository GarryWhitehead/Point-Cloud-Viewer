#include "RenderPass.h"

#include "VulkanAPI/Image.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkContext.h"
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


vk::AttachmentLoadOp RenderPass::loadFlagsToVk(const LoadType flags)
{
    vk::AttachmentLoadOp result;
    switch (flags)
    {
        case LoadType::Clear:
            result = vk::AttachmentLoadOp::eClear;
            break;
        case LoadType::DontCare:
            result = vk::AttachmentLoadOp::eDontCare;
            break;
        case LoadType::Store:
            result = vk::AttachmentLoadOp::eLoad;
            break;
    }
    return result;
}

vk::AttachmentStoreOp RenderPass::storeFlagsToVk(const StoreType flags)
{
    vk::AttachmentStoreOp result;
    switch (flags)
    {
        case StoreType::DontCare:
            result = vk::AttachmentStoreOp::eDontCare;
            break;
        case StoreType::Store:
            result = vk::AttachmentStoreOp::eStore;
            break;
    }
    return result;
}

vk::SampleCountFlagBits RenderPass::samplesToVk(const uint32_t count)
{
    vk::SampleCountFlagBits result = vk::SampleCountFlagBits::e1;
    switch (count)
    {
        case 1:
            result = vk::SampleCountFlagBits::e1;
            break;
        case 2:
            result = vk::SampleCountFlagBits::e2;
            break;
        case 4:
            result = vk::SampleCountFlagBits::e4;
            break;
        case 8:
            result = vk::SampleCountFlagBits::e8;
            break;
        case 16:
            result = vk::SampleCountFlagBits::e16;
            break;
        case 32:
            result = vk::SampleCountFlagBits::e32;
            break;
        case 64:
            result = vk::SampleCountFlagBits::e64;
            break;
        default:
            LOGGER_WARN("Unsupported sample count. Set to one.");
            break;
    }
    return result;
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

void RenderPass::addOutputAttachment(
    const vk::Format format,
    const uint32_t reference,
    ClearFlags& clearFlags,
    const uint32_t sampleCount)
{
    auto iter = std::find_if(
        outputRefs.begin(), outputRefs.end(), [&reference](const OutputReferenceInfo& lhs) {
            return lhs.ref == reference;
        });

    // if an output reference with the same id has already been added, just return
    if (iter != outputRefs.end())
    {
        return;
    }

    vk::AttachmentDescription attachDescr;
    attachDescr.format = format;
    attachDescr.initialLayout = vk::ImageLayout::eUndefined;
    attachDescr.finalLayout = getFinalTransitionLayout(format);

    // samples
    attachDescr.samples = samplesToVk(sampleCount);

    // clear flags
    attachDescr.loadOp = loadFlagsToVk(clearFlags.attachLoad); // pre image state
    attachDescr.storeOp = storeFlagsToVk(clearFlags.attachStore); // post image state
    attachDescr.stencilLoadOp = loadFlagsToVk(clearFlags.stencilLoad); // pre stencil state
    attachDescr.stencilStoreOp = storeFlagsToVk(clearFlags.stencilStore); // post stencil state

    attachments.emplace_back(attachDescr);

    // reference to the attachment
    OutputReferenceInfo info;
    info.ref.attachment = reference;
    info.ref.layout = getAttachmentLayout(format);
    info.index = attachments.size() - 1;

    outputRefs.emplace_back(info);
}

void RenderPass::addInputRef(const uint32_t reference)
{
    // obtain the layout from the output ref - this also acts as a sanity check that a reader
    // has a writer
    vk::AttachmentReference ref;
    ref.attachment = reference;
    auto iter = std::find_if(
        outputRefs.begin(), outputRefs.end(), [&reference](const OutputReferenceInfo& lhs) {
            return lhs.ref == reference;
        });

    // if a reference with the same id has already been added, just return
    if (iter != outputRefs.end())
    {
        return;
    }

    ref.layout = iter->ref.layout;
    inputRefs.emplace_back(ref);
}

bool RenderPass::addSubPass(
    std::vector<uint32_t>& reqInputRefs,
    std::vector<uint32_t>& reqOutputRefs,
    const uint32_t reqDepthRef)
{
    // override default subpass with user specified subpass
    SubpassInfo subpass;
    subpass.descr.pipelineBindPoint =
        vk::PipelineBindPoint::eGraphics; // only graphic renderpasses supported at present

    // sort out the colour refs
    for (const uint32_t ref : reqOutputRefs)
    {
        auto iter = std::find_if(
            outputRefs.begin(), outputRefs.end(), [&ref](const OutputReferenceInfo& lhs) {
                return lhs.ref.attachment == ref;
            });

        if (iter == outputRefs.end())
        {
            LOGGER_ERROR("Inavlid colour attachment reference.");
            return false;
        }

        subpass.colourRefs.emplace_back(iter->ref);
    }

    // and the input refs
    for (const uint32_t ref : reqInputRefs)
    {
        auto iter = std::find_if(
            inputRefs.begin(), inputRefs.end(), [&ref](const vk::AttachmentReference& lhs) {
                return lhs.attachment == ref;
            });

        if (iter == inputRefs.end())
        {
            LOGGER_ERROR("Inavlid input attachment reference.");
            return false;
        }

        subpass.inputRefs.emplace_back(*iter);
    }

    // and the dpeth if required
    if (reqDepthRef != UINT32_MAX)
    {
        auto iter = std::find_if(
            outputRefs.begin(), outputRefs.end(), [&reqDepthRef](const OutputReferenceInfo& lhs) {
                return lhs.ref.attachment == reqDepthRef;
            });

        if (iter == outputRefs.end())
        {
            LOGGER_ERROR("Inavlid depth attachment reference.");
            return false;
        }
        subpass.depth = &iter->ref;
    }

    subpasses.emplace_back(subpass);
    return true;
}

void RenderPass::addSubpassDependency(const Util::BitSetEnum<VulkanAPI::SubpassFlags>& flags)
{
    vk::SubpassDependency depend[2];
    depend[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    if (flags.testBit(SubpassFlags::TopOfPipeline))
    {
        depend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        depend[0].dstSubpass = 0;
        depend[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        depend[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;

        if (flags.testBit(SubpassFlags::ColourRead))
        {
            depend[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            depend[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        }
        else if (
            (flags.testBit(SubpassFlags::DepthRead)) && flags.testBit(SubpassFlags::StencilRead))
        {
            depend[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            depend[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
        else if (flags.testBit(SubpassFlags::StencilRead))
        {
            depend[0].dstStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
            depend[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
    }
    else if (flags.testBit(SubpassFlags::BottomOfPipeline))
    {
        depend[0].srcSubpass = 0;
        depend[0].dstSubpass = VK_SUBPASS_EXTERNAL;
        depend[0].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        depend[0].dstAccessMask = vk::AccessFlagBits::eShaderRead;

        if (flags.testBit(SubpassFlags::Merged))
        {
            depend[0].srcSubpass = 0;
        }

        if (flags.testBit(SubpassFlags::ColourRead))
        {
            depend[0].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            depend[0].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        }
        else if (flags.testBit(SubpassFlags::DepthRead) && flags.testBit(SubpassFlags::StencilRead))
        {
            depend[0].srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            depend[0].srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
        else if (flags.testBit(SubpassFlags::StencilRead))
        {
            depend[0].srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
            depend[0].srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
    }
    else
    {
        if (!flags.testBit(SubpassFlags::Merged))
        {
            depend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            depend[0].dstSubpass = 0;
        }
    }

    dependencies.emplace_back(depend[0]);

    // and the next dependency stage
    depend[1].srcSubpass = depend[0].dstSubpass;
    depend[1].dstSubpass = depend[0].srcSubpass;
    depend[1].srcStageMask = depend[0].dstStageMask;
    depend[1].dstStageMask = depend[0].srcStageMask;
    depend[1].srcAccessMask = depend[0].dstAccessMask;
    depend[1].dstAccessMask = depend[0].srcAccessMask;
    
    dependencies.emplace_back(depend[1]);
}

void RenderPass::prepare()
{
    // copy all the subpass declerations into one container
    assert(!subpasses.empty());

    std::vector<vk::SubpassDescription> sDescr;
    for (auto& sp : subpasses)
    {
        // finalise the subpasses now we know no more passes are going to be added
        sp.descr.colorAttachmentCount = static_cast<uint32_t>(sp.colourRefs.size());
        sp.descr.pColorAttachments = sp.colourRefs.data();

        // input attachments
        sp.descr.inputAttachmentCount = static_cast<uint32_t>(sp.inputRefs.size());
        sp.descr.pInputAttachments = sp.inputRefs.data();

        // depth attachment - if required
        if (sp.depth != nullptr)
        {
            sp.descr.pDepthStencilAttachment = sp.depth;
        }

        sDescr.emplace_back(sp.descr);
    }

    vk::RenderPassCreateInfo createInfo(
        {},
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        static_cast<uint32_t>(sDescr.size()),
        sDescr.data(),
        static_cast<uint32_t>(dependencies.size()),
        dependencies.data());

    VK_CHECK_RESULT(context.device.createRenderPass(&createInfo, nullptr, &renderpass));
}

vk::RenderPass& RenderPass::get()
{
    assert(renderpass);
    return renderpass;
}

uint32_t RenderPass::getWidth() const
{
    return width;
}

uint32_t RenderPass::getHeight() const
{
    return height;
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

    vk::FramebufferCreateInfo frameInfo {
        {},
        rpass.get(),
        static_cast<uint32_t>(views.size()),
        views.data(),
        width,
        height,
        layerCount};

    VK_CHECK_RESULT(context.device.createFramebuffer(&frameInfo, nullptr, &fbuffer));
}


} // namespace VulkanAPI