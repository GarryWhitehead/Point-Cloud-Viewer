CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
CMAKE_POLICY(VERSION 3.5)

PROJECT(PCV LANGUAGES CXX C)
SET(CMAKE_CXX_STANDARD 17)
 
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
 
################################################################################################################
# compiler specific flags
################################################################################################################

IF(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Wall -Wextra -Wno-missing-field-initializers")
ELSEIF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -D_CRT_NONSTDC_NO_DEPRECATE -MP")
ENDIF()

#platform specific compiler settings
IF(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES=1")
ENDIF()

################################################################################################################
# paths
################################################################################################################

# the cmake path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# root cmake directory
SET(PCV_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# external directory path
SET(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)

SET(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)

IF(APPLE)
    SET(MOLTENVK_ROOT "NOT_DEFINED" CACHE PATH "Path to the MoltenVk repository.")
    IF(${MOLTENVK_ROOT} STREQUAL "NOT_DEFINED")
        MESSAGE(FATAL_ERROR "The path to the MoltenVk repository must be defined.")
    ENDIF()
ENDIF()

##############################################################################################################
# Find vulkan libs
##############################################################################################################

# find vulkan using cmake module if the correct cmake version
IF(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using cmake version which supports FindVulkan. Using this module to find Vulkan")
	find_package(Vulkan)
	SET(FOUND_VULKAN ON)
ELSE()
	IF(WIN32)
		FIND_LIBRARY(VULKAN_LIB NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF(VULKAN_LIB)
			SET(FOUND_VULKAN ON)
			MESSAGE("Found Vulkan library.")
		ENDIF()
	ELSE()
		FIND_LIBRARY(VULKAN_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_CURRENT_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF(VULKAN_LIB)
			SET(FOUND_VULKAN ON)
			MESSAGE("Found Vulkan library.")
		ENDIF()
	ENDIF()
ENDIF()

IF(NOT FOUND_VULKAN)
	MESSAGE(FATAL_ERROR "Unable to find Vulkan library.")
ELSE()
	MESSAGE(STATUS ${VULKAN_LIB})
ENDIF()

SET(LIBVK "Vulkan::Vulkan")

###############################################################################################################
# External libraries
###############################################################################################################

ADD_SUBDIRECTORY(external/glfw)

###############################################################################################################
# Setup MoltenVk if MacOS
###############################################################################################################
# Use a bash script to copy files and create export paths

IF(APPLE)
	EXECUTE_PROCESS(
		COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK-Setup.sh"
		RESULT_VARIABLE CMD_ERROR
        OUTPUT_VARIABLE OUTPUT_MSG
	)
    MESSAGE(${OUTPUT_MSG})
ENDIF()

# build the library
ADD_SUBDIRECTORY(PCV)



