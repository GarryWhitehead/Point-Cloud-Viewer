#include "NativeWindowWrapper.h"

namespace OmegaEngine
{

void* WindowInstance::getNativeWindowPtr()
{
	return nativeWin;
}

uint32_t WindowInstance::getWidth() const
{
	return width;
}

uint32_t WindowInstance::getHeight() const
{
	return height;
}

}    // namespace OmegaEngine