#pragma once

#include "Application/NativeWindowWrapper.h"
#include "Vulkan/Common.h"
#include "Vulkan/Image.h"
#include "Vulkan/Platform/Surface.h"
#include "Vulkan/RenderPass.h"

#include <vector>

namespace OmegaEngine
{
class OEWindowInstance;
}

namespace VulkanAPI
{

// forward declerations
class VkDriver;

struct SwapchainContext
{
    ImageView view;
    vk::CommandBuffer commands;
    vk::Fence fence;
};

class Swapchain
{

public:

    Swapchain();
    ~Swapchain();

    /**
     * @brief Creates the swapchain using the supplied native window surface
     * Note: The surface is obtained by calling **createSurface**.
     * Note: This function must be called before using with a **Scene** object.
     * @param context A prepared vulkan device object
     * @param surface A surface object for the given platform
     */
    bool prepare(VkContext& context, Platform::SurfaceWrapper& surface);

    // static functions
    /**
     * @brief Creates a KHR surface object using a native window pointer.
     * Note: The native window is obtained from a source such as glfw (used in the application for
     * now)
     * @param window A wrapper containg all the info needed to create a surface and swapchain
     * @param instance A vulkan instance wrapper obtained from calling **createInstance**
     */
    static Platform::SurfaceWrapper
    createSurface(OmegaEngine::OEWindowInstance* window, vk::Instance& instance);

    vk::SwapchainKHR& get();
    uint32_t getExtentsHeight() const;
    uint32_t getExtentsWidth() const;
    ImageView& getImageView(const uint8_t index);
    
    friend class VkDriver;

private:
    /// creates the image views for the swapchain
    void prepareImageViews(VkContext& context, const vk::SurfaceFormatKHR& surfaceFormat);

private:
    // the dimensions of the current swapchain
    vk::Extent2D extent;

    // a swapchain based on the present surface type
    vk::SwapchainKHR swapchain;

    std::vector<SwapchainContext> contexts;
};
} // namespace VulkanAPI
