#include "Core/Engine.h"

#include "Core/Scene.h"
#include "Rendering/Renderer.h"

#include "Application/NativeWindowWrapper.h"

#include "Vulkan/SwapChain.h"

namespace PCV
{

Engine::Engine()
{
}

Engine::~Engine()
{
    for (Renderer* rend : renderers)
    {
        if (rend)
        {
            delete rend;
        }
    }
    renderers.clear();
}

bool Engine::init(OEWindowInstance* window)
{
    if (!vkDriver->createInstance(window->extensions.first, window->extensions.second))
    {
        LOGGER_ERROR("Fatal Error whilst creating Vulkan instance.");
        return false;
    }

    surface = VulkanAPI::Swapchain::createSurface(window, vkDriver->getContext().instance);

    if (!vkDriver->init(surface.get()))
    {
        LOGGER_ERROR("Fatal Error whilst preparing vulkan device.");
        return false;
    }
    return true;
}

SwapchainHandle Engine::createSwapchain(OEWindowInstance* window)
{
    // create a swapchain for surface rendering based on the platform specific window surface
    auto sc = std::make_unique<VulkanAPI::Swapchain>();
    sc->prepare(vkDriver->getContext(), surface);
    swapchains.emplace_back(std::move(sc));
    return SwapchainHandle {static_cast<uint32_t>(swapchains.size() - 1)};
}

Renderer* Engine::createRenderer(SwapchainHandle& handle, OEScene* scene)
{
    auto& swapchain = swapchains[handle.getHandle()];

    OERenderer* renderer = new OERenderer(*this, *scene, *swapchain, config);
    assert(renderer);
    renderers.emplace_back(renderer);
    return renderer;
}


} // namespace OmegaEngine
