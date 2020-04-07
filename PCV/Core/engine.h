#pragma once

#include "Vulkan/Platform/Surface.h"

#include <memory>
#include <vector>

namespace PCV
{
// forward declerations
class OEWorld;;
class Renderer;
class OEScene;
class EngineConfig;
class OEWindowInstance;

class Engine 
{
public:
    
    // for now the default values for engine config are stored here
    static constexpr float Default_ClearVal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    
	Engine();
	~Engine();

	/**
	* @brief Initialises a new vulkan context. This creates a new instance and 
	* prepares the physical and abstract device and associated queues
	* Note: Only one vulkan device is allowed. Multiple devices supporting multi-gpu
	* setups is not yet supported
	*/
	bool init(OEWindowInstance* window);

	/**
	* @brief This creates a new swapchain instance based upon the platform-specific
	* ntaive window pointer created by the application
	*/
	SwapchainHandle createSwapchain(OEWindowInstance* window);
	
	/**
	* @brief Creates a new renderer instance based on the user specified swapchain and scene
	*/
	Renderer* createRenderer(SwapchainHandle& handle, OEScene* scene);

private:
 
    // A list of renderers which have been created
    std::vector<Renderer*> renderers;
  
    VulkanAPI::Platform::SurfaceWrapper surface;
    
	// keep a list of active swapchains here
	std::vector<std::unique_ptr<VulkanAPI::Swapchain>> swapchains;

};

}    // namespace OmegaEngine
