Cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
project("PloverRaycaster" VERSION "0.1.0" DESCRIPTION "A plover-based raycaster.")

if (WIN32)
	add_compile_options(-finstrument-functions)
endif()

if (LINUX)
	add_compile_options(-finstrument-functions-after-inlining)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
endif()

set(GLM_ENABLE_CXX_20 "ON")
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG  bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.0
)
FetchContent_MakeAvailable(glm)

add_library(PloverRaycaster SHARED "src/Main.cpp" "src/plover-raycaster.h")
add_subdirectory("plover")

message(STATUS $ENV{SHADER_COMPILE_SCRIPT})

add_custom_target(compile_shaders ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/$ENV{SHADER_COMPILE_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Compiling shaders")

add_custom_target(copy_resources ALL
    DEPENDS compile_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/resources
    ${PROJECT_BINARY_DIR}/resources
    COMMENT "Copy resources into binary's directory")

add_custom_target ( compile_tags ALL
    COMMAND ctags src/ plover/src lapwing/src -R --c++-kinds=+p --fields=+iaS --extras=+q 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling project tags")

add_dependencies(${PROJECT_NAME} compile_shaders)
add_dependencies(${PROJECT_NAME} copy_resources)
add_dependencies(${PROJECT_NAME} compile_tags)

target_include_directories(${PROJECT_NAME}
    PUBLIC plover/include
)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)

