#pragma once

#include <memory>
#include <cstdint>
#include <vector>

namespace OmegaEngine
{

class WindowInstance 
{
public:
    
    void* getNativeWindowPtr();

    uint32_t getWidth() const;
    
    uint32_t getHeight() const;

    friend class Engine;
    friend class Application;
    
private:
    
    void* nativeWin = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	std::pair<const char**, uint32_t> extensions;
    
};

}
