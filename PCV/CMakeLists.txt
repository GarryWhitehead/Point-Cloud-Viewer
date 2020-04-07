

# Generate as a static library 
ADD_LIBRARY(PCV_LIB STATIC "")

# ================ compiler ============================
TARGET_COMPILE_OPTIONS(PCV_LIB PRIVATE ${PCV_CXX_FLAGS})

# =================== definitions ===================
# flags for vulkan surface platform
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(PCV_LIB PUBLIC VK_USE_PLATFORM_WIN32_KHR)
	MESSAGE("Building for win32 KHR surface...")
 ELSEIF(APPLE)
    TARGET_COMPILE_DEFINITIONS(PCV_LIB PUBLIC VK_USE_PLATFORM_MACOS_MVK)
    MESSAGE("Building for macos surface....")
ENDIF()

# Shader and assets path definitions
TARGET_COMPILE_DEFINITIONS(PCV_LIB PUBLIC OE_SHADER_DIR=\"${SHADER_DIR}/\")

# ================= include targets ====================
TARGET_INCLUDE_DIRECTORIES(PCV_LIB
    PUBLIC
	${EXTERNAL_DIR}
	${EXTERNAL_DIR}/vulkan
	${EXTERNAL_DIR}/VulkanMemoryAllocator/src

	PRIVATE
	Source
	${CMAKE_CURRENT_SOURCE_DIR}/Application
	${CMAKE_CURRENT_SOURCE_DIR}/Application/Platforms
	${CMAKE_CURRENT_SOURCE_DIR}/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Components
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Rendering
	${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanAPI
	${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanAPI/Platform
	${CMAKE_CURRENT_SOURCE_DIR}
)

# =============== source files =============================
TARGET_SOURCES(PCV_LIB 

	PRIVATE
	Application/Application.cpp Application/Application.h
	Application/Platforms/PlatformGlfw.cpp Application/Platforms/PlatformGlfw.h
	Application/NativeWindowWrapper.cpp Application/NativeWindowWrapper.h 

	Core/Engine.cpp Core/Engine.h
	Core/Scene.cpp Core/Scene.h
    Core/Camera.cpp Core/Camera.h

	Rendering/RenderQueue.cpp Rendering/RenderQueue.h
	Rendering/Renderer.cpp Rendering/Renderer.h
   	
	VulkanAPI/Platform/Surface.h
    VulkanAPI/Platform/Surface_Win32.cpp
    VulkanAPI/Platform/Surface_Linux.cpp
    VulkanAPI/Platform/Surface_Cocoa.mm

	VulkanAPI/VkContext.cpp VulkanAPI/VkContext.h
	VulkanAPI/Pipeline.cpp VulkanAPI/Pipeline.h
	VulkanAPI/RenderPass.cpp VulkanAPI/RenderPass.h
	VulkanAPI/SwapChain.cpp VulkanAPI/SwapChain.h
)

# ================= linking =======================

IF(APPLE AND NOT IOS)
    TARGET_LINK_LIBRARIES(PCV_LIB PRIVATE "-framework Cocoa")
ENDIF()

TARGET_LINK_LIBRARIES(PCV_LIB 
	PRIVATE
	${Vulkan_LIBRARY} 
	glfw 
)

