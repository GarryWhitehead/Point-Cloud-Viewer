#include "Application.h"

#include "NativeWindowWrapper.h"
#include "Core/engine.h"
#include "Core/Scene.h"
#include "Rendering/Renderer.h"

#include <cassert>
#include <thread>

using namespace std::literals::chrono_literals;

namespace OmegaEngine
{

Application* Application::create(const char* title, uint32_t width, uint32_t height)
{
    // create a new instance
    Application* app = new Application();

    if (!app->init(title, width, height))
    {
        printf("Fatal Error. Unbale to create an instance pf OE application.");
        return nullptr;
    }

    return app;
}

Engine* Application::createEngine(WindowInstance* window)
{
    OEEngine* eng = new OEEngine();
    if (!eng)
    {
        printf("Unable to crerate an engine instance");
        return nullptr;
    }
    if (!eng->init(window))
    {
        printf("Fatal Error. Unable to initialiase the engine.");
        return nullptr;
    }
    return eng;
}

WindowInstance* Application::init(const char* title, uint32_t width, uint32_t height)
{
    // init glfw
    if (!glfw.init())
    {
        printf("Unable to initilaise glfw.");
        return nullptr;
    }

    // create a window
    if (!glfw.createWindow(width, height, title))
    {
        printf("Unable to create a glfw window.");
        return nullptr;
    }

    winInstance = new WindowInstance();
    assert(winInstance);

    winInstance->width = width;
    winInstance->height = height;
    winInstance->nativeWin = (void*) glfw.getNativeWinPointer();
    winInstance->extensions = glfw.getInstanceExt();

    return winInstance;
}

bool Application::run(Scene* scene, Renderer* renderer)
{
    // convert delta time to ms
    const NanoSeconds frameTime(33ms);

    Util::Timer<NanoSeconds> timer;

    NanoSeconds appStartTime = timer.getCurrentTime();

    while (!closeApp)
    {
        NanoSeconds frameTime = timer.getCurrentTime();

        // check for any input from the window
        glfw.poll();

        // update the scene
        if (!scene->update(frameTime.count()))
        {
            return false;
        }

        // and the renderer
        if (!renderer->update())
        {
            return false;
        }

        // user define pre-render callback to be added here (or virtual)

        // TODP: multi view option- with each view drawn.
        // begin the rendering for this frame
        renderer->draw();

        // user defined post-render callback to be added here

        // calculate whether we have any time remaining this frame
        NanoSeconds endTime = timer.getCurrentTime();
        NanoSeconds elapsedTime = endTime - frameTime;

        // if we haven't used up the frame time, sleep for remainder
        if (elapsedTime < frameTime)
        {
            std::this_thread::sleep_for(frameTime - elapsedTime);
        }
    }
    return true;
}

void Application::destroy(OEApplication* app)
{
    if (app)
    {
        delete app;
        app = nullptr;
    }
}

OEWindowInstance* Application::getWindow()
{
    assert(winInstance);
    return winInstance;
}

} // namespace OmegaEngine
