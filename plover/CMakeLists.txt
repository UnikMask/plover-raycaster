cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)

project("plover")

include(GNUInstallDirs)
include(FetchContent)

if (WIN32)
        message(STATUS ">>> WIN32")
	set(WIN32_MAIN WIN32)
	set(MAIN_FILE "src/win32_plover.cpp")
    set(INST_FILE "src/win32_instrument.cpp")
    set(PLAT_PREFIX "win32")
	add_compile_options(-finstrument-functions)
endif()

if (LINUX)
    message(STATUS ">>> LINUX")
    set(PLAT_PREFIX "linux")
	add_compile_options(-finstrument-functions-after-inlining)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
	add_link_options(-Wl,-E)
endif()


# Dependencies
set(Vulkan_LIBRARY $ENV{Vulkan_LIBRARY})
set(FREETYPE_DIR $ENV{Freetype_DIR})
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)

FetchContent_Declare(
    glm 
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG e2c92645460f680fd272fd2eed591efb2be7dc31
)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)


add_executable(
    ${PROJECT_NAME}
    ${WIN32_MAIN}
    "src/${PLAT_PREFIX}_plover.cpp" "src/${PLAT_PREFIX}_plover.h"
    "src/${PLAT_PREFIX}_instrument.cpp" "src/instrument.h"
    "include/plover/plover.h"
    "src/plover.cpp" "src/plover_int.h"
    "src/Renderer.cpp" "src/Renderer.h"
    "src/VulkanContext.cpp" "src/VulkanContext.h"
    "src/DescriptorAllocator.cpp" "src/DescriptorAllocator.h"
    "src/Mesh.cpp" "src/Mesh.h"
    "src/Material.cpp" "src/Material.h"
    "src/ttfRenderer.cpp" "src/ttfRenderer.h"
    "src/Texture.cpp" "src/Texture.h"
    "src/UI.cpp" "src/UI.h"
    "src/AssetLoader.h" "src/AssetLoader.cpp"
    )

if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy "C:/NorthheadDeps/lib/freetype.dll" ${CMAKE_CURRENT_BINARY_DIR}/.
    )
endif ()

add_subdirectory(libraries/header_libs)

target_include_directories(${PROJECT_NAME}
	PUBLIC include/
	PUBLIC ../lapwing/include
	PUBLIC ${Vulkan_INCLUDE_DIRS}
	PUBLIC libraries/header_libs/include
	PUBLIC ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC Vulkan::Vulkan
	PUBLIC glfw
	PUBLIC glm::glm
	PUBLIC ${FREETYPE_LIBRARIES}
)

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC dbghelp)
endif()
