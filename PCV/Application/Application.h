#pragma once

#include "Platforms/PlatformGlfw.h"

#include <cstdint>

// forward declerations
namespace OmegaEngine
{

class Engine;
class Scene;
class Renderer;
class WindowInstance;

class Application
{
    
public:
    
    Application() = default;

    static Application* create(const char* title, uint32_t width, uint32_t height);

    static void destroy(Application* app);

    Engine* createEngine(WindowInstance* window);

    /** 
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param: Title to use for the window. Nullptr states no title bar
     * @param width: window width in dpi; if zero will sets window width to fullscreen size 
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns a native window pointer
    */
	WindowInstance* init(const char* title, uint32_t width, uint32_t height);

    bool run(Scene* scene, Renderer* renderer);

    OEWindowInstance* getWindow();

private:

    // A engine instance. Only one permitted at the moment.
	Engine* engine = nullptr;

    // current window - maybe we should allow multiple window instances?
	WindowInstance* winInstance = nullptr;

    GlfwPlatform glfw;

    // the running state of this app. Set to true by 'esc' keypress or window close
    bool closeApp = false;
};

}
